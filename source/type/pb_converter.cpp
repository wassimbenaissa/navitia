#include "pb_converter.h"

namespace nt = navitia::type;
namespace navitia{

/**
 * fonction générique pour convertir un objet navitia en un message protocol buffer
 *
 * @param idx identifiant de l'objet à convertir
 * @param data reférence vers l'objet Data de l'application
 * @param message l'objet protocol buffer a remplir
 * @param depth profondeur de remplissage
 *
 * @throw std::out_of_range si l'idx n'est pas valide
 * @throw std::bad_cast si le message PB n'est pas adapté
 */
template<nt::Type_e type>
void fill_pb_object(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth = 0){throw std::exception();}

template<>
void fill_pb_object<nt::Type_e::eCity>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int){
    pbnavitia::City* city = dynamic_cast<pbnavitia::City*>(message);
    nt::City city_n = data.pt_data.cities.at(idx);
    city->set_id(city_n.id);
    city->set_id(city_n.id);
    city->set_external_code(city_n.external_code);
    city->set_name(city_n.name);
    city->mutable_coord()->set_x(city_n.coord.x);
    city->mutable_coord()->set_y(city_n.coord.y);
}

/**
 * spécialisation de fill_pb_object pour les StopArea
 *
 */
template<>
void fill_pb_object<nt::Type_e::eStopArea>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::StopArea* stop_area = dynamic_cast<pbnavitia::StopArea*>(message);
    nt::StopArea sa = data.pt_data.stop_areas.at(idx);
    stop_area->set_id(sa.id);
    stop_area->set_external_code(sa.external_code);
    stop_area->set_name(sa.name);
    stop_area->mutable_coord()->set_x(sa.coord.x);
    stop_area->mutable_coord()->set_y(sa.coord.y);
    if(max_depth > 0){
        try{
            fill_pb_object<nt::Type_e::eCity>(sa.city_idx, data, stop_area->mutable_child()->add_city(), max_depth-1);
        }catch(std::out_of_range e){}
    }
}

/**
 * spécialisation de fill_pb_object pour les StopArea
 *
 */
template<>
void fill_pb_object<nt::Type_e::eStopPoint>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::StopPoint* stop_point = dynamic_cast<pbnavitia::StopPoint*>(message);
    nt::StopPoint sp = data.pt_data.stop_points.at(idx);
    stop_point->set_id(sp.id);
    stop_point->set_external_code(sp.external_code);
    stop_point->set_name(sp.name);
    stop_point->mutable_coord()->set_x(sp.coord.x);
    stop_point->mutable_coord()->set_y(sp.coord.y);
    if(max_depth > 0){
        try{
            fill_pb_object<nt::Type_e::eCity>(sp.city_idx, data, stop_point->mutable_child()->add_city(), max_depth-1);
        }catch(std::out_of_range e){}
        
    }
}

/**
 * spécialisation de fill_pb_object pour les Way
 *
 */
template<>
void fill_pb_object<nt::Type_e::eWay>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::Way* way = dynamic_cast<pbnavitia::Way*>(message);
    navitia::streetnetwork::Way w = data.street_network.ways.at(idx);
    way->set_name(w.name);
    /*stop_point->mutable_coord()->set_x(sp.coord.x);
    stop_point->mutable_coord()->set_y(sp.coord.y);*/
    if(max_depth > 0){
        try{
            fill_pb_object<nt::Type_e::eCity>(w.city_idx, data, way->mutable_child()->add_city(), max_depth-1);
        }catch(std::out_of_range e){
            std::cout << w.city_idx << std::endl;
        }
        
    }
}
template<>
void fill_pb_object<nt::Type_e::eLine>(nt::idx_t idx, const nt::Data& data, google::protobuf::Message* message, int max_depth){
    pbnavitia::Line* line = dynamic_cast<pbnavitia::Line*>(message);
    navitia::type::Line l = data.pt_data.lines.at(idx);
    line->set_forward_name(l.forward_name);
    line->set_backward_name(l.backward_name);
    line->set_code(l.code);
    line->set_color(l.color);
    line->set_name(l.name);
}

}//namespace navitia
