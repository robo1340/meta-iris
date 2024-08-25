let map;
var first_map_update = false;
let my_location_marker;
var markers = {}; //a dictionary of location markers for other users


var numberIcon = L.divIcon({
                    className: "NumberIcon",
                    iconSize: [25, 41],
                    iconAnchor: [10, 44],
                    popupAnchor: [3, -40],
					//html:'<span class="my-div-span">11111111111111111111 1111111111111111111</span>'  
					html:'1'  
              });

//const myIcon = L.icon({iconUrl: '/node_modules/leaflet/dist/images/marker-icon.png',});

const myIcon = L.icon({
   iconUrl: '/marker_custom2.png',
});

function set_map_location(lat_lon_arr){
	//console.log(lat_lon_arr)
	map.setView(lat_lon_arr, 14);
	map.flyTo(lat_lon_arr, 14);	

	my_location_marker = L.marker(lat_lon_arr, {icon: myIcon}).addTo(map);
	my_location_marker.bindTooltip(my_username, {permanent: true});
}

const map_position_update_cb = (position) => {
	var lat = position.coords.latitude;
	var lon = position.coords.longitude;
	lat  = 37.54557;
	lon = -97.26893;
	
	my_location = {'username':my_username, 'coords':[lat,lon] }
	//console.log(my_location);
	socket.emit('location', my_location)
	updateUser(my_username, lat=lat, lon=lon)
	send_location_beacon();
	
	if (first_map_update == false){
		first_map_update = true;
		set_map_location([lat, lon]);
	} else {
		my_location_marker.setLatLng([lat, lon]);
	}
}

function other_user_message_cb(data){
	console.log("other_user_message_cb");
	addMessage(data.username, data.message);
	
	if (notifications == true){
		const notification = new Notification(data.username + ": " + data.message);
	}
}

function other_user_location_cb(data){
	console.log("other_user_location_cb");
	updateUser(data.username, data.coords[0], data.coords[1]);
	
	var new_marker = L.marker(data.coords, ).addTo(map);
	new_marker.bindTooltip(data.username, {permanent: true});
}


var lastGrey = false;

function add_user_to_list(user, index){
    var li = $("<li></li>",{
			"class": "user",
			"id" : user.username,
			"style": "background-color:" + (lastGrey ? "white" : "#eee") + ";list-style-type:none;",
			"text": index + ". " + user.username + ": " + iso_to_time(user.last_location_beacon_time)
		});
    $("#userList").append(li);
    lastGrey = !lastGrey;
    //scroll to bottom of div with scroll bar
    //setting position:fixed for this div is important
    //in adding a scroll bar to it
    $("#userPane").scrollTop($("#userPane")[0].scrollHeight);		
}

function users_loaded_cb(users){
	//console.log(users)
	for (i in users) {
		var index = parseInt(i)+1
		var numberIcon = L.divIcon({
							className: "NumberIcon",
							iconSize: [25, 41],
							iconAnchor: [10, 44],
							popupAnchor: [3, -40],
							//html:'<span class="my-div-span">11111111111111111111 1111111111111111111</span>'  
							html:index
					  });
		markers[users[i].username] = L.marker([users[i].lat, users[i].lon], {icon: numberIcon}).addTo(map);
		markers[users[i].username].bindTooltip(
			users[i].username + ' \n' + users[i].last_location_beacon_time, 
			{permanent: false, direction: 'right'}
		);
		add_user_to_list(users[i], index)
	}
	
}

function msg_db_cb() {
	return;
}

function user_db_cb() {
	load_users(users_loaded_cb);
	return;
}

function toChat(e) {
	console.log('toChat()')
	window.location = '/new.html';
}

function toSettings(e){
	console.log('toSettings()')
}

$(document).ready(function() {
	$('button[name=toChat]').click(toChat);

	map = L.map('map');

	var vectorTileStyling = {
		water: {
			fill: true,
			weight: 1,
			fillColor: '#06cccc',
			color: '#06cccc',
			fillOpacity: 0.2,
			opacity: 0.4,
		},
		admin: {
			weight: 1,
			fillColor: 'pink',
			color: 'pink',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		waterway: {
			weight: 1,
			fillColor: '#2375e0',
			color: '#2375e0',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		landcover: {
			fill: true,
			weight: 1,
			fillColor: '#53e033',
			color: '#53e033',
			fillOpacity: 0.2,
			opacity: 0.4,
		},
		landuse: {
			fill: true,
			weight: 1,
			fillColor: '#e5b404',
			color: '#e5b404',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		park: {
			fill: true,
			weight: 1,
			fillColor: '#84ea5b',
			color: '#84ea5b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		boundary: {
			weight: 1,
			fillColor: '#c545d3',
			color: '#c545d3',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		aeroway: {
			weight: 1,
			fillColor: '#51aeb5',
			color: '#51aeb5',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		transportation: {	// openmaptiles only
			weight: 0.5,
			fillColor: '#f2b648',
			color: '#f2b648',
			fillOpacity: 0.2,
			opacity: 0.4,
	// 					dashArray: [4, 4]
		},
		building: {
			fill: true,
			weight: 1,
			fillColor: '#2b2b2b',
			color: '#2b2b2b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		water_name: {
			weight: 1,
			fillColor: '#022c5b',
			color: '#022c5b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		transportation_name: {
			weight: 1,
			fillColor: '#bc6b38',
			color: '#bc6b38',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		place: {
			weight: 1,
			fillColor: '#f20e93',
			color: '#f20e93',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		housenumber: {
			weight: 1,
			fillColor: '#ef4c8b',
			color: '#ef4c8b',
			fillOpacity: 0.2,
			opacity: 0.4
		},
		poi: {
			weight: 1,
			fillColor: '#3bb50a',
			color: '#3bb50a',
			fillOpacity: 0.2,
			opacity: 0.4
		},


		// Do not symbolize some stuff for mapbox
		//country_label: [],
		//marine_label: [],
		//state_label: [],
		//place_label: [],
		//waterway_label: [],
		//poi_label: [],
		//road_label: [],
		//housenum_label: [],

		// Do not symbolize some stuff for openmaptiles
		country_name: {
			weight: 1
		},
		marine_name: {
			weight: 1
		},
		state_name: {
			weight: 100,
			fill: true,
			color: '#c0c0c0',
		},
		place_name: {
			weight: 1
		},
		waterway_name: {
			weight: 1
		},
		poi_name: {
			weight: 1
		},
		road_name: {
			weight: 1
		},
		housenum_name: {
			weight: 1
		},
		point: { color: "transparent" },
	};


	var openmaptilesVectorTileOptions = {
		rendererFactory: L.canvas.tile,
		attribution: '<a href="https://openmaptiles.org/">&copy; OpenMapTiles</a>, <a href="http://www.openstreetmap.org/copyright">&copy; OpenStreetMap</a> contributors',
		vectorTileLayerStyles: vectorTileStyling,
		maxZoom: 14,
		minZoom: 14,
		//getFeatureId: function (e) {return}
	};

	var tile_url = "/tiles/{z}/{x}/{y}.pbf";
	var tile_grid = L.vectorGrid.protobuf(tile_url, openmaptilesVectorTileOptions)
	tile_grid.addTo(map);


	//var popup = L.popup()
	//	.setLatLng([34.468611, -97.521389])
	//	.setContent("I am a standalone popup.")
	//	.openOn(map);

	//L.tileLayer('http://tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);

	/*
	var pbfLayer = L.vectorGrid.protobuf(openmaptilesUrl, vectorTileOptions)
		.on('click', function(e) {	// The .on method attaches an event handler
			L.popup()
				.setContent(e.layer.properties.name || e.layer.properties.type)
				.setLatLng(e.latlng)
				.openOn(map);

			L.DomEvent.stop(e);
		})
		.addTo(map);
	*/

	//var marker = L.marker([35.468611, -97.521389]).addTo(map);
	//marker.bindPopup("<b>Hello world!</b><br>I am a popup.").openPopup();

	my_username = Cookies.get('username');
	if (!my_username){
		my_username = "Me"
	} else {
		const notification = new Notification("Welcome to maps: " + my_username);
	}
	setup_db(msg_db_cb, user_db_cb);
	setup_location_watch(map_position_update_cb);
	setup_notifications();
	
	setup_socket_callbacks(other_user_message_cb, other_user_location_cb);
	setup_location_beacon();

	

	/*
	var startTime = new Date().getTime();
	map.on('movestart', function(evt) {
		startTime = new Date().getTime();
	});

	map.on('moveend', function(evt) {
		endTime = new Date().getTime();
		console.log('elapsed: ' + (endTime - startTime));
	});
	*/
	
});