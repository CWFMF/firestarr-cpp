<!DOCTYPE html>
        <html lang="en">
          <head>
            <meta charset="utf-8">
            <meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no' />
            <title>FireSTARR Outputs</title>

            <!-- Leaflet -->
            <link rel="stylesheet" href="//npmcdn.com/leaflet@1.2.0/dist/leaflet.css" />
            <script src="//npmcdn.com/leaflet@1.2.0/dist/leaflet.js"></script>
            <script src="//d3js.org/d3.v4.min.js"></script>
            <script src="https://unpkg.com/esri-leaflet@3.0.2/dist/esri-leaflet.js"
                integrity="sha512-myckXhaJsP7Q7MZva03Tfme/MSF5a6HC2xryjAM4FxPLHGqlh5VALCbywHnzs2uPoF/4G/QVXyYDDSkp5nPfig=="
                crossorigin=""></script>
            <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>
            <script src="betterWMS.js"></script>
            <style>
                body { margin:0; padding:0; }
                body, table, tr, td, th, div, h1, h2, input { font-family: "Calibri", "Trebuchet MS", "Ubuntu", Serif; font-size: 11pt; }
                #map { position:absolute; top:0; bottom:0; width:100%; } /* full size */
                .ctl {
                    padding: 2px 10px 2px 10px;
                    background: white;
                    background: rgba(255,255,255,0.9);
                    box-shadow: 0 0 15px rgba(0,0,0,0.2);
                    border-radius: 5px;
                    text-align: right;
                }
                .title {
                    font-size: 18pt;
                    font-weight: bold;
                }
                .src {
                    font-size: 10pt;
                }

            </style>

        </head>
        <body>

        <div id="map"></div>

        <script>
        /* **** Leaflet **** */

        // Base layers
        //  .. OpenStreetMap
        var osm = L.tileLayer('http://{s}.tile.osm.org/{z}/{x}/{y}.png', {attribution: '&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'});

        //  .. CartoDB Positron
        var cartodb = L.tileLayer('http://{s}.basemaps.cartocdn.com/light_all/{z}/{x}/{y}.png', {attribution: '&copy; <a href="http://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors, &copy; <a href="http://cartodb.com/attributions">CartoDB</a>'});

        //  .. OSM Toner
        var toner = L.tileLayer('http://{s}.tile.stamen.com/toner/{z}/{x}/{y}.png', {attribution: 'Map tiles by <a href="http://stamen.com">Stamen Design</a>, under <a href="http://creativecommons.org/licenses/by/3.0">CC BY 3.0</a>. Data by <a href="http://openstreetmap.org">OpenStreetMap</a>, under <a href="http://www.openstreetmap.org/copyright">ODbL</a>.'});

        //  .. White background
        var white = L.tileLayer("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAQAAAAEAAQMAAABmvDolAAAAA1BMVEX///+nxBvIAAAAH0lEQVQYGe3BAQ0AAADCIPunfg43YAAAAAAAAAAA5wIhAAAB9aK9BAAAAABJRU5ErkJggg==");

        // Overlay layers (TMS)
        var prob = L.tileLayer('/data/output/probability/tiled/{z}/{x}/{y}.png', {tms: true, opacity: 0.7, attribution: ""});
        var perim = L.tileLayer('/data/output/perimeter/tiled/{z}/{x}/{y}.png', {tms: true, opacity: 0.7, attribution: ""});
        var bc_fires = L.esri.featureLayer({'url': 'https://services6.arcgis.com/ubm4tcTYICKBpist/ArcGIS/rest/services/BCWS_FirePerimeters_PublicView/FeatureServer/0'});
        var CWFIS_WMS = 'https://cwfis.cfs.nrcan.gc.ca/geoserver/public/wms?';
        var fire_m3 = L.tileLayer.betterWms(CWFIS_WMS, {
                                        'layers': 'public:m3_polygons_current',
                                        'transparent': true,
                                        'format': 'image/png',
                                        'cql_filter': 'lastdate>=2021-08-30T00:00:00'
                                      });
        var active = L.tileLayer.betterWms(CWFIS_WMS, {
                                        'layers': 'public:activefires_current',
                                        'transparent': true,
                                        'format': 'image/png',
                                      });
        // Map
        var map = L.map('map', {
            center: [53.90794041375133, -122.35548907293926],
            zoom: 12,
            minZoom: 5,
            maxZoom: 12,
            layers: [osm]
        });

        map.fitBounds([[45, -50], [64, -141]]);
        var basemaps = {"OpenStreetMap": osm, "CartoDB Positron": cartodb, "Stamen Toner": toner, "Without background": white}
        var overlaymaps = {"Probability": prob, "Perimeter": perim, 'BC Fires': bc_fires, 'Fire M3': fire_m3, 'Active Fires': active}

        // Title
        var title = L.control();
        title.onAdd = function(map) {
            this._div = L.DomUtil.create('div', 'ctl title');
            this.update();
            return this._div;
        };
        title.update = function(props) {
            this._div.innerHTML = "FireSTARR Outputs";
        };
        title.addTo(map);

        // Note
        var src = 'Generated by <a href="http://www.klokan.cz/projects/gdal2tiles/">GDAL2Tiles</a>, Copyright &copy; 2008 <a href="http://www.klokan.cz/">Klokan Petr Pridal</a>,  <a href="http://www.gdal.org/">GDAL</a> &amp; <a href="http://www.osgeo.org/">OSGeo</a> <a href="http://code.google.com/soc/">GSoC</a>';
        var title = L.control({position: 'bottomleft'});
        title.onAdd = function(map) {
            this._div = L.DomUtil.create('div', 'ctl src');
            this.update();
            return this._div;
        };
        title.update = function(props) {
            this._div.innerHTML = src;
        };
        title.addTo(map);


        // Add base layers
        L.control.layers(basemaps, overlaymaps, {collapsed: false}).addTo(map);


        </script>

        </body>
        </html>

        