
const WAYPOINT_CHEVRON = `
			<svg
			  width="20"
			  height="26"
			  viewBox="0 0 4 4"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 C 1 1 3 1 4 0" fill="#7A8BE7"></path>
			</svg>`;
			
const TRIANGLE = `
			<svg
			  width="20"
			  height="26"
			  viewBox="0 0 4 4"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 0 0 L 4 0 L 2 4 L 0 0" fill="#7A8BE7"></path>
			</svg>`;
			
const CROWN = `
			<svg
			  width="20"
			  height="26"
			  viewBox="0 0 4 4"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 L 4 1 L 2 4 L 0 1 L 0 0 L 1 1 L 2 0 L 3 1 L 4 0" fill="#7A8BE7"></path>
			</svg>`;
			
const SQUARE = `
			<svg
			  width="20"
			  height="26"
			  viewBox="0 0 4 4"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 L 4 4 L 0 4 L 0 0 L 4 0" fill="#7A8BE7"></path>
			</svg>`;

const WAYPOINT_FLAT = `
			<svg
			  width="20"
			  height="26"
			  viewBox="0 0 4 4"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 4 0" fill="#7A8BE7"></path>
			</svg>`;

const WAYPOINT_CROWN = `
			<svg
			  width="20"
			  height="26"
			  viewBox="-1 -1 5 5"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 0 -1 L 1 0 L 2 -1 L 3 0 L 4 -1 L 4 0 Z" fill="#7A8BE7"></path>
			</svg>`;

const WAYPOINT_ROUNDED = `
			<svg
			  width="20"
			  height="26"
			  viewBox="-2 -2 6 6"
			  version="1.1"
			  preserveAspectRatio="none"
			  xmlns="http://www.w3.org/2000/svg"
			>
				<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 4 0 A 1 1 0 0 0 0 0" fill="#7A8BE7"></path>
			</svg>`;

SVG = {
	"WAYPOINT_CHEVRON" 	: WAYPOINT_CHEVRON,
	"TRIANGLE" 			: TRIANGLE,
	"CROWN" 			: CROWN,
	"SQUARE" 			: SQUARE,
	"WAYPOINT_FLAT" 	: WAYPOINT_FLAT,
	"WAYPOINT_CROWN" 	: WAYPOINT_CROWN,
	"WAYPOINT_ROUNDED" 	: WAYPOINT_ROUNDED
};

var waypoints = {}; //a dictionary of location waypoints sent by users
let waypoint_db = undefined;

const HR = '<hr style="margin-top:5px;margin-bottom:5px;border-color:black">';

function retrive_waypoint_message(){
	var waypoint_msg = Cookies.get('waypoint_msg');
	if (waypoint_msg === undefined){
		waypoint_msg = "";
		Cookies.set('waypoint_msg',waypoint_msg, {expires:COOKIE_EXP});
	}
	return waypoint_msg;
}

var waypoint_period = 0;
function retrive_waypoint_period(){
	waypoint_period = Cookies.get('waypoint_period');
	if (waypoint_period === undefined){
		waypoint_period = 0;
		Cookies.set('waypoint_period',waypoint_period, {expires:COOKIE_EXP});
	}
	return waypoint_period;
}

function retrive_waypoint_color(){
	var waypoint_color = Cookies.get('waypoint_color');
	if (waypoint_color === undefined){
		waypoint_color = WAYPOINT_BUTTON_COLORS[0];
		Cookies.set('waypoint_color',waypoint_color, {expires:COOKIE_EXP});
	}
	return waypoint_color;
}

const WAYPOINT_BUTTON_PERIODS=[0,5,10,20,30,60,120];
const WAYPOINT_BUTTON_COLORS=["blue","green","orange","red","black"];
const WAYPOINT_BUTTON_COLOR_CODES={
	"blue":"B",
	"green":"G",
	"orange":"O",
	"red":"R",
	"black":"B"
};

function generate_waypoint_period_buttons(periods=WAYPOINT_BUTTON_PERIODS){
	var c = retrive_waypoint_period();
	var to_return = HR + '<h4>Waypoint Period (seconds)</h4><div class="container">';
	for (let i in periods){
		var cls = (periods[i] == c) ? '"map_context_button active"' : '"map_context_button"';
		to_return += '<button id="'+periods[i]+'_period" class='+cls+'>'+periods[i]+'</button>';
	}
	to_return += "</div>";
	return to_return;
}

function generate_waypoint_period_button_events(periods=WAYPOINT_BUTTON_PERIODS){
	for (i in periods){
		document.getElementById(periods[i]+'_period').addEventListener('click', function(e){
			for (j in periods){
				var b = document.getElementById(periods[j]+'_period');
				b.className = b.className.replace(" active","");
			}
			e.srcElement.className += " active";
			Cookies.set('waypoint_period',e.srcElement.innerHTML, {expires:COOKIE_EXP});
			messenger.postMessage(["start_waypoint_beacon",retrive_waypoint_period()]);
		});
	}
}

function generate_waypoint_color_buttons(colors=WAYPOINT_BUTTON_COLORS){
	var c = retrive_waypoint_color();
	var to_return = HR + '<h4>Waypoint Color</h4><div class="container">';
	for (let i in colors){
		var cls = (colors[i] == c) ? '"map_context_button map_context_button_color active "' : '"map_context_button map_context_button_color"';
		to_return += '<button id="'+colors[i]+'_color" class='+cls+ 'style="background-color:'+colors[i]+';"' +'>'+WAYPOINT_BUTTON_COLOR_CODES[colors[i]]+'</button>';
	}
	to_return += "</div>";
	return to_return;
}

function generate_waypoint_color_button_events(colors=WAYPOINT_BUTTON_COLORS){
	for (i in colors){
		document.getElementById(colors[i]+'_color').addEventListener('click', function(e){
			for (j in colors){
				var b = document.getElementById(colors[j]+'_color');
				b.className = b.className.replace(" active","");
			}
			e.srcElement.className += " active";
			var new_color = e.srcElement.id.split('_')[0];
			Cookies.set('waypoint_color',new_color, {expires:COOKIE_EXP});
			updateWaypoint(my_username, coords=null, msg=null, timestamp=null, color=new_color, function(w){
				if (w.username in waypoints){
					waypoints[w.username]._icon.style.fill = w.color;
					
					messenger.postMessage(["start_waypoint_beacon",retrive_waypoint_period()]);
					messenger.postMessage(["waypoint",{'username': w.username, 'msg': w.msg, 'coords':w.coords, 'color':w.color}]);
				}
			});		
		});
	}
}

function send_waypoint(coords, msg=""){
	updateWaypoint(my_username, coords, msg=msg, timestamp=null, color=retrive_waypoint_color(), function(w){
		if (w.username in waypoints){
			waypoints[w.username].setLatLng(coords);
			waypoints[w.username]._tooltip._content = w.username + ' ' + iso_to_time(w.time) + ' ' + w.msg;
			waypoints[w.username]._icon.style.fill = w.color;
		} else {
			//const i = Object.keys(waypoints).length+1;
			waypoints[w.username] = L.marker(w.coords, {icon: waypoint_icon_factory()}).addTo(map);
			waypoints[w.username]._icon.style.fill = w.color;
			waypoints[w.username].bindTooltip(
				'My Waypoint ' + iso_to_time(w.time) + ' ' + w.msg,
				{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
			);
		}
		messenger.postMessage(["start_waypoint_beacon",retrive_waypoint_period()]);
		messenger.postMessage(["waypoint",{'username': w.username, 'msg': w.msg, 'coords':w.coords, 'color':w.color}]);
	});	
}

function setup_waypoints(map){
	waypoint_db_init();
	
	map.on('contextmenu', (e) => {
		//console.log('Lat, Lon:', e.latlng.lat + ',' + e.latlng.lng);
		var coords = e.latlng;
		const id = coords.toString();
		btn1 = '<button id="place_waypoint" class="map_context_button">Place Waypoint Here</button>'
		btn2 = '<button id="msg_waypoint" class="map_context_button">Message Waypoint Here</button>'
		btn3 = '<button id="cancel_waypoint" class="map_context_button">Cancel Waypoint Beacon</button>'
		var popup = L.popup(coords, options={
			"closeButton":false,
			"class" : "map_context_popup"
		})
		.setContent(btn1 + btn2 + generate_waypoint_period_buttons() + generate_waypoint_color_buttons() + HR + btn3)
		.openOn(map);  
		
		generate_waypoint_period_button_events();
		generate_waypoint_color_button_events();
		
		// We bind the click event here in the same scope as we add the Popup
		document.getElementById('place_waypoint').addEventListener('click', function(e){
			//console.log("place_marker");
			//console.log(coords);
			send_waypoint(coords);
			popup.close();
		});
		
		// We bind the click event here in the same scope as we add the Popup
		document.getElementById('msg_waypoint').addEventListener('click', function(e){
			//console.log("place_marker");
			//console.log(coords);
			let msg = prompt("Waypoint Message", retrive_waypoint_message());
			if (msg != ""){Cookies.set('waypoint_msg',msg, {expires:COOKIE_EXP});}
			send_waypoint(coords, msg);
			popup.close();
		});	
		
		document.getElementById('cancel_waypoint').addEventListener('click', function(e){
			console.log("cancel waypoint");
			messenger.postMessage(["stop_waypoint_beacon",null]);
			updateWaypoint(my_username, coords, msg="", timestamp=null, color=null, function(w){
				if (w.username in waypoints){
					waypoints[w.username].remove();
					delete waypoints[w.username];
				}
			});
			popup.close();
		});
		
	});	
}

function waypoint_icon_factory(){
	const class_name = "waypoint_icon";
	var to_return = L.divIcon({
		html: SVG['WAYPOINT_CHEVRON'],
		className: class_name,
		iconSize: [26, 26],
		iconAnchor: [10, 26]
	});
	return to_return
}

////////////////////////////////////////////////////////////////
////////////////////Database Operations/////////////////////////
////////////////////////////////////////////////////////////////

function addWaypoint(username, coords, msg, timestamp, color="blue"){
	if (waypoint_db === undefined){return;}
	
	const objectStore = get_waypoint_object_store();
	const addRequest = objectStore.add({
		username:username, 
		coords:coords,
		time:timestamp,
		msg:msg,
		color:color
	});		
}

function float_equals(f1, f2, delta=0.01){
	//console.log("float_equals("+f1+","+f2+")");
	//console.log(Math.abs(f1-f2));
	//console.log(delta + " " + (Math.abs(f1-f2) < delta));
	return (Math.abs(f1-f2) < delta);
}

function updateWaypoint(username, coords, msg="", timestamp=null, color=null, complete_cb=do_nothing){
	if (waypoint_db === undefined){return;}
	if (timestamp == null){
		timestamp = get_iso_timestamp();
	}
	var changed = false;
	const objectStore = get_waypoint_object_store();
	const read_request = objectStore.get(username);
	read_request.onsuccess = function() {
		if (read_request.result === undefined){
			addWaypoint(username, coords, msg, timestamp, color);
			complete_cb({
				username:username, 
				coords:coords,
				time:timestamp,
				msg:msg,
				color:color
				},
				true
			);
		} else {
			var old = read_request.result;
			var new_color = (color===null) ? old.color : color;
			var new_msg = (msg === null) ? old.msg : msg;
			var new_coords = (coords === null) ? old.coords : coords;

			//console.log((new_color != old.color));
			//console.log((new_msg != old.msg));
			//console.log(!float_equals(old.coords.lat, new_coords.lat));
			//console.log(!float_equals(old.coords.lng, new_coords.lng));
			//console.log(new_color + " : " + old.color);
			
			changed = ((new_color != old.color) || (new_msg != old.msg) 
				|| !float_equals(old.coords.lat, new_coords.lat) 
				|| !float_equals(old.coords.lng, new_coords.lng));
			
			//console.log(new_color);
			old.username = username;
			old.coords = new_coords;
			old.time = timestamp;
			old.msg = new_msg;
			old.color = new_color;
			const addRequest = get_waypoint_object_store().put(old);
			//addRequest.onerror = (event) => {console.log("Do something with the error");};
			//addRequest.onsuccess = (event) => {console.log("Success - the data is updated!");};
			complete_cb(old,changed);
		}
		
    };
}

function load_waypoints(finished_cb=waypoints_loaded_cb) {
	if (waypoint_db !== undefined){
		const objectStore = get_waypoint_object_store();
		var messages = objectStore.getAll();
		messages.onsuccess = function() {
			finished_cb(messages.result);
		};
	}
}

function waypoints_loaded_cb(w){
	//console.log(users)
	for (i in w) {
		waypoints[w[i].username] = L.marker(w[i].coords, {icon: waypoint_icon_factory()}).addTo(map);
		waypoints[w[i].username].bindTooltip(
			w[i].username + ' STALE ' + w[i].msg,
			{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
		);
		if (waypoints[w[i].username]._icon !== undefined){
			waypoints[w[i].username]._icon.style.fill = w[i].color;
		}
		if (w[i].username == my_username){
			messenger.postMessage(["start_waypoint_beacon",retrive_waypoint_period()]);
			messenger.postMessage(["waypoint",{'username': w[i].username, 'msg': w[i].msg, 'coords':w[i].coords, 'color':w[i].color}]);
		}
	}	
}

function other_user_waypoint_cb(data, timestamp){
	if (data.username == my_username){return;}
	console.log("other_user_waypoint_cb");
	updateWaypoint(data.username, data.coords, data.msg, timestamp, data.color, function(w, changed){
		txt = w.username + ' ' + iso_to_time(w.time) + ' ' + w.msg
		if (w.username in waypoints){
			waypoints[data.username].setLatLng(data.coords);
			waypoints[data.username]._tooltip._content = txt;
			waypoints[data.username]._icon.style.fill = w.color;
		} else {
			const i = Object.keys(waypoints).length+1;
			waypoints[data.username] = L.marker(data.coords, {icon: waypoint_icon_factory()}).addTo(map);
			waypoints[data.username]._icon.style.fill = w.color;
			waypoints[data.username].bindTooltip(txt,{permanent: false, direction: 'right', className: 'leaflet-tooltip'});	
		}	
		
		//need to make this popup more conditional
		if (changed == true){
			var popup = L.popup({className: "leaflet-tooltip"}).setLatLng(data.coords).setContent('<p>'+txt+'</p>').openOn(map);
			setTimeout(function(){popup.close()}, 1500);
		}
		set_tab_pending("map");
	});
}

function clear_waypoints(finished_cb=do_nothing) {
	if (waypoint_db === undefined){return;}	
	get_waypoint_object_store().clear();
	finished_cb();
}

function get_waypoint_object_store(){
	return waypoint_db.transaction(["waypoints"], "readwrite").objectStore("waypoints");
}

function waypoint_db_init(waypoint_db_cb=load_waypoints) {
	const way_open_req  = window.indexedDB.open("waypoint_db", 2);
	
	way_open_req.addEventListener("error", () =>
		console.error("way_open_req failed to open"),
	);
	
	way_open_req.addEventListener("success", () => {
		waypoint_db = way_open_req.result;
		waypoint_db_cb();
	});
	
	way_open_req.addEventListener("upgradeneeded", (e) => {
		console.log("Upgrade waypoint_db");
		waypoint_db = e.target.result;

		// Create an objectStore in our database to store notes and an auto-incrementing key
		// An objectStore is similar to a 'table' in a relational database
		const objectStore = waypoint_db.createObjectStore("waypoints", {
			keyPath: "username",
			autoIncrement: true,
		});

		// Define what data items the objectStore will contain
		objectStore.createIndex("username", "username", {unique: false });
		objectStore.createIndex("coords", "coords", 	{unique: false });
		objectStore.createIndex("msg", "msg", 	{unique: false });
		objectStore.createIndex("time", "time", 		{unique: false });
		objectStore.createIndex("color", "color", 		{unique: false });

		console.log("Database setup complete");
	});
	
}