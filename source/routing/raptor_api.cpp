#include "raptor_api.h"
#include "type/pb_converter.h"
#include "boost/date_time/posix_time/posix_time.hpp"
namespace navitia { namespace routing { namespace raptor {

std::string iso_string(const nt::Data & d, int date, int hour){
    boost::posix_time::ptime date_time(d.meta.production_date.begin() + boost::gregorian::days(date));
    date_time += boost::posix_time::seconds(hour);
    return boost::posix_time::to_iso_extended_string(date_time);
}

pbnavitia::Response make_pathes(const std::vector<navitia::routing::Path> &paths, const nt::Data & d) {
    pbnavitia::Response pb_response;
    pb_response.set_requested_api(pbnavitia::PLANNER);

    for(Path path : paths) {
        //navitia::routing::Path itineraire = navitia::routing::makeItineraire(path);
        pbnavitia::Journey * pb_journey = pb_response.mutable_planner()->add_journey();
        pb_journey->set_duration(path.duration);
        pb_journey->set_nb_transfers(path.nb_changes);
        for(PathItem & item : path.items){
            pbnavitia::Section * pb_section = pb_journey->add_section();
            pb_section->set_arrival_date_time(iso_string(d, item.arrival.date(), item.arrival.hour()));
            pb_section->set_departure_date_time(iso_string(d, item.departure.date(), item.departure.hour()));
            if(item.type == public_transport)
                pb_section->set_type(pbnavitia::PUBLIC_TRANSPORT);
            else
                pb_section->set_type(pbnavitia::TRANSFER);
            if(item.type == public_transport && item.vj_idx != type::invalid_idx){
                const type::VehicleJourney & vj = d.pt_data.vehicle_journeys[item.vj_idx];
                const type::Route & route = d.pt_data.routes[vj.route_idx];
                const type::Line & line = d.pt_data.lines[route.line_idx];
                fill_pb_object<type::Type_e::eLine>(line.idx, d, pb_section->mutable_line());
            }
            for(navitia::type::idx_t stop_point : item.stop_points){
                fill_pb_object<type::Type_e::eStopPoint>(stop_point, d, pb_section->add_stop_point());
            }
            if(item.stop_points.size() >= 2) {
                pbnavitia::PlaceMark * origin_place_mark = pb_section->mutable_origin();
                origin_place_mark->set_type(pbnavitia::STOPAREA);
                fill_pb_object<type::Type_e::eStopArea>(d.pt_data.stop_points[item.stop_points.front()].stop_area_idx, d, origin_place_mark->mutable_stop_area());
                pbnavitia::PlaceMark * destination_place_mark = pb_section->mutable_destination();
                destination_place_mark->set_type(pbnavitia::STOPAREA);
                fill_pb_object<type::Type_e::eStopArea>(d.pt_data.stop_points[item.stop_points.back()].stop_area_idx, d, destination_place_mark->mutable_stop_area());
            }

        }
    }

    return pb_response;
}

std::vector<std::pair<type::idx_t, double> > get_stop_points(const type::EntryPoint &ep, const type::Data & data, streetnetwork::StreetNetworkWorker & worker){
    std::vector<std::pair<type::idx_t, double> > result;

    switch(ep.type) {
    case navitia::type::Type_e::eStopArea:
    {
        auto it = data.pt_data.stop_area_map.find(ep.external_code);
        if(it!= data.pt_data.stop_area_map.end()) {
            for(auto spidx : data.pt_data.stop_areas[it->second].stop_point_list) {
                result.push_back(std::make_pair(spidx, 0));
            }
        }
    } break;
    case type::Type_e::eStopPoint: {
        auto it = data.pt_data.stop_point_map.find(ep.external_code);
        if(it != data.pt_data.stop_point_map.end()){
            result.push_back(std::make_pair(data.pt_data.stop_points[it->second].idx, 0));
        }
    } break;
    case type::Type_e::eCoord: {
        result = worker.find_nearest(ep.coordinates, data.pt_data.stop_point_proximity_list, 300);
    } break;
    default: break;
    }
    return result;
}

vector_idxretour to_idxretour(std::vector<std::pair<type::idx_t, double> > elements, int hour, int day){
    vector_idxretour result;
    for(auto item : elements) {
        int temps = hour + (item.second / 80);
        int jour;
        if(temps > 86400) {
            temps = temps % 86400;
            jour = day + 1;
        } else {
            jour = day;
        }
        result.push_back(std::make_pair(item.first, type_retour(navitia::type::invalid_idx, DateTime(jour, temps), 0, (item.second / 80))));
    }
    return result;
}


pbnavitia::Response make_response(RAPTOR &raptor, const type::EntryPoint &departure, const type::EntryPoint &destination, int time, const boost::gregorian::date &date, const senscompute sens, streetnetwork::StreetNetworkWorker & worker) {
    pbnavitia::Response response;
    if(time < 0 || time > 24*3600){
        response.set_error("Invalid hour");
        return response;
    }

    int day = (date - raptor.data.meta.production_date.begin()).days();
    if(day < 0 || day > raptor.data.meta.production_date.length().days()){
        response.set_error("Invalid date");
        return response;
    }

    if(!raptor.data.meta.production_date.contains(date)) {
        response.set_error("Date not in the production period");
        return response;
    }

    auto departures = get_stop_points(departure, raptor.data, worker);
    if(departures.size() == 0){
        response.set_error("Departure point not found");
        return response;
    }

    auto destinations = get_stop_points(destination, raptor.data, worker);
    if(destinations.size() == 0){
        response.set_error("Destination point not found");
        return response;
    }

    std::vector<Path> result;

    if(sens == partirapres)
        result = raptor.compute_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day));
    else
        result = raptor.compute_reverse_all(to_idxretour(departures, time, day), to_idxretour(destinations, time, day));

    return make_pathes(result, raptor.data);
}

}}}
