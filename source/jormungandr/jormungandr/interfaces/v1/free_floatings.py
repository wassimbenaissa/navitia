# coding=utf-8

# Copyright (c) 2001-2021, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# channel `#navitia` on riot https://riot.im/app/#/room/#navitia:matrix.org
# https://groups.google.com/d/forum/navitia
# www.navitia.io

from __future__ import absolute_import, print_function, unicode_literals, division
from flask_restful import abort
from jormungandr.interfaces.v1.serializer.free_floating import FreeFloatingsSerializer
from jormungandr import i_manager, timezone
from jormungandr.interfaces.v1.ResourceUri import ResourceUri
from jormungandr.interfaces.parsers import default_count_arg_type
from jormungandr.interfaces.v1.transform_id import transform_id
from jormungandr.scenarios.utils import free_floatings_type
from navitiacommon.parser_args_type import OptionValue, CoordFormat
from jormungandr.instance import Instance
from typing import Optional, Dict


def build_instance_shape(instance):
    # type: (Instance) -> Optional[Dict]
    if instance and instance.geojson:
        return {"type": "Feature", "properties": {}, "geometry": instance.geojson}
    return None


class FreeFloatingsNearby(ResourceUri):
    def __init__(self, *args, **kwargs):
        ResourceUri.__init__(self, output_type_serializer=FreeFloatingsSerializer, *args, **kwargs)
        parser_get = self.parsers["get"]
        parser_get.add_argument(
            "type[]",
            type=OptionValue(free_floatings_type),
            action="append",
            help="Type of free-floating objects to return",
        )
        parser_get.add_argument("distance", type=int, default=500, help="Distance range of the query in meters")
        parser_get.add_argument("count", type=default_count_arg_type, default=10, help="Elements per page")
        self.parsers['get'].add_argument(
            "coord",
            type=CoordFormat(nullable=True),
            help="Coordinates longitude;latitude used to search " "the objects around this coordinate",
        )
        parser_get.add_argument("start_page", type=int, default=0, help="The current page")

    def options(self, **kwargs):
        return self.api_description(**kwargs)

    def get(self, region=None, lon=None, lat=None, uri=None):
        self.region = i_manager.get_region(region, lon, lat)
        timezone.set_request_timezone(self.region)
        args = self.parsers["get"].parse_args()
        instance = i_manager.instances.get(self.region)

        # coord in the form of uri:
        # /coverage/<coverage name>/coord=<lon;lat>/freefloatings_nearby?...
        if uri:
            if uri[-1] == '/':
                uri = uri[:-1]
            uris = uri.split("/")
            if len(uris) >= 2 and uris[-2] == 'coord':
                args["coord"] = transform_id(uris[-1])
            else:
                abort(404)
        # coord as parameter: /coverage/<coverage name>/freefloatings_nearby?coord=<lon;lat>&...
        else:
            coord = args.get("coord")
            if coord is None:
                abort(404)
        self._register_interpreted_parameters(args)
        resp = instance.external_service_provider_manager.manage_free_floatings('free_floatings', args)

        return resp, 200
