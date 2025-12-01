

class Icon {
	
	static waypoint_chevron(color='green') {
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="0 0 4 4"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 C 1 1 3 1 4 0" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}
	
	static triangle(color='green') {
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="0 0 4 4"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 0 0 L 4 0 L 2 4 L 0 0" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}
	
	static crown(color='green') {	
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="0 0 4 4"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 4 0 L 4 1 L 2 4 L 0 1 L 0 0 L 1 1 L 2 0 L 3 1 L 4 0" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}
	
	static square(color='green') {			
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="0 0 4 4"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 4 0 L 4 4 L 0 4 L 0 0 L 4 0" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}
	
	static waypoint_flat(color='green') {
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="0 0 4 4"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 4 0" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}

	static waypoint_crown(color='green') {
		const to_return = `
				<svg
				  width="20"
				  height="26"
				  viewBox="-1 -1 5 5"
				  version="1.1"
				  preserveAspectRatio="none"
				  fill=${color}
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 0 -1 L 1 0 L 2 -1 L 3 0 L 4 -1 L 4 0 Z" fill="#7A8BE7"></path>
				</svg>`;
		return to_return;
	}

	static waypoint_rounded(color='green') {
		const to_return = `
					<svg
					  width="20"
					  height="26"
					  viewBox="-2 -2 6 6"
					  version="1.1"
					  preserveAspectRatio="none"
					  fill=${color}
					  xmlns="http://www.w3.org/2000/svg"
					>
						<path d="M 4 0 C 4 2 3 3 2 4 C 1 3 0 2 0 0 L 4 0 A 1 1 0 0 0 0 0" fill="#7A8BE7"></path>
					</svg>`;
		return to_return;
		
	}
}

SVG = {
	"WAYPOINT_CHEVRON" 	: Icon.waypoint_chevron,
	"TRIANGLE" 			: Icon.triangle,
	"CROWN" 			: Icon.crown,
	"SQUARE" 			: Icon.square,
	"WAYPOINT_FLAT" 	: Icon.waypoint_flat,
	"WAYPOINT_CROWN" 	: Icon.waypoint_crown,
	"WAYPOINT_ROUNDED" 	: Icon.waypoint_rounded
};

var waypoints = {}; //a dictionary of location waypoints sent by users
let waypoint_db;

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
	return Util.get_cookie('waypoint_period', 0, parseInt);
}

function retrive_waypoint_color(){
	return Util.get_cookie('waypoint_color', WAYPOINT_BUTTON_COLORS[0]);
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
			Waypoint.get(state.my_addr, function(w){
				messenger.postMessage(["waypoint",{'src': state.my_addr, 'dst':0, 'msg': w.msg, 'lat':w.lat, 'lon':w.lon, 'color':w.color, 'cancel':false, 'period':retrive_waypoint_period()}]);
			});
			
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
			Waypoint.update(state.my_addr, null, null, null, null, color=new_color, function(w){
				if (w.addr in waypoints){
					waypoints[w.addr].setIcon(WaypointView.icon(w.color));
					messenger.postMessage(["waypoint",{'src': state.my_addr, 'dst':0, 'msg': w.msg, 'lat':w.lat, 'lon':w.lon, 'color':w.color, 'cancel':false, 'period':retrive_waypoint_period()}]);
				}
			});		
		});
	}
}

class Waypoint {
	//e.latlng.lat + ',' + e.latlng.lng
	static send(lat, lon, msg=""){
		console.log('Waypoint.send',lat,lon,msg);
		Waypoint.update(state.my_addr, lat, lon, msg, null, retrive_waypoint_color(), function(w){
			var c = L.latLng(lat, lon);
			if (w.addr in waypoints){
				waypoints[w.addr].setLatLng(c);
				waypoints[w.addr]._tooltip._content = state.my_callsign + ' ' + Util.iso_to_time(w.time) + ' ' + w.msg;
				waypoints[w.addr].setIcon(WaypointView.icon(w.color));
			} else {
				//const i = Object.keys(waypoints).length+1;
				waypoints[w.addr] = L.marker(c, {icon: WaypointView.icon()}).addTo(map);
				waypoints[w.addr].setIcon(WaypointView.icon(w.color));
				waypoints[w.addr].bindTooltip(
					'My Waypoint ' + Util.iso_to_time(w.time) + ' ' + w.msg,
					{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
				);
			}
			messenger.postMessage(["waypoint",{'src': state.my_addr, 'dst':0, 'msg': w.msg, 'lat':w.lat, 'lon':w.lon, 'color':w.color, 'cancel':false, 'period':retrive_waypoint_period()}]);
			
		});	
	}
	
	static add(addr, lat, lon, msg, timestamp, color="blue"){
		if (waypoint_db === undefined){return;}
		
		const objectStore = Waypoint.object_store();
		const addRequest = objectStore.add({
			addr:addr, 
			lat:lat,
			lon:lon,
			time:timestamp,
			msg:msg,
			color:color
		});		
	}
	
	static get(addr, cb) {
		if (waypoint_db === undefined) {return false;}
		if (typeof addr === "string" || addr instanceof String) {
			addr = parseInt(addr,10);
		}
		const objectStore = Waypoint.object_store();
		const read_request = objectStore.get(addr);
		read_request.onsuccess = function() {
			if (read_request.result === undefined){return false;}//{cb(null);}
			return cb(read_request.result);
		};	
	}

	static update(addr, lat=null, lon=null, msg="", timestamp=null, color=null, complete_cb=do_nothing){
		if (addr === undefined) {return;}
		console.log('Waypoint.update',addr,lat,lon,msg,timestamp,color);
		if (waypoint_db === undefined){return;}
		if (timestamp == null){
			timestamp = Util.get_iso_timestamp();
		}
		var changed = false;
		const objectStore = Waypoint.object_store();
		const read_request = objectStore.get(addr);
		read_request.onsuccess = function() {
			if (read_request.result === undefined){
				Waypoint.add(addr, lat, lon, msg, timestamp, color);
				complete_cb({
					addr:addr, 
					lat:lat,
					lon:lon,
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
				var new_lat = (lat === null) ? old.lat : lat;
				var new_lon = (lon === null) ? old.lon : lon;

				//console.log((new_color != old.color));
				//console.log((new_msg != old.msg));
				//console.log(!Util.float_equals(old.coords.lat, new_coords.lat));
				//console.log(!Util.float_equals(old.coords.lng, new_coords.lng));
				//console.log(new_color + " : " + old.color);
				//console.log(new_color + " : " + old.color);
				
				changed = ((new_color != old.color) || (new_msg != old.msg) 
					|| !Util.float_equals(old.lat, new_lat) 
					|| !Util.float_equals(old.lon, new_lon));
				
				//console.log(new_color);
				old.addr = addr;
				old.lat = new_lat;
				old.lon = new_lon;
				old.time = timestamp;
				old.msg = new_msg;
				old.color = new_color;
				const addRequest = Waypoint.object_store().put(old);
				//addRequest.onerror = (event) => {console.log("Do something with the error");};
				//addRequest.onsuccess = (event) => {console.log("Success - the data is updated!");};
				complete_cb(old,changed);
			}
			
		};
	}
	
	static clear(finished_cb=do_nothing) {
		if (waypoint_db === undefined){return;}	
		Waypoint.object_store().clear();
		finished_cb();
	}

	static object_store(){
		return waypoint_db.transaction(["waypoints"], "readwrite").objectStore("waypoints");
	}
	
	static load(finished_cb=WaypointView.loaded_cb) {
		if (waypoint_db !== undefined){
			const objectStore = Waypoint.object_store();
			var messages = objectStore.getAll();
			messages.onsuccess = function() {
				finished_cb(messages.result);
			};
		}
	}
	
}

class WaypointView {
	
	constructor(map) {
		this.map = map;
		
		this.map.on('contextmenu', (e) => {
			//console.log('Lat, Lon:', e.latlng.lat + ',' + e.latlng.lng);
			//console.log(this.map);
			var lat = e.latlng.lat;
			var lon = e.latlng.lng;
			const id = e.latlng.toString();
			var btn1 = '<button id="place_waypoint" class="map_context_button">Place Waypoint Here</button>'
			var btn2 = '<button id="msg_waypoint" class="map_context_button">Message Waypoint Here</button>'
			var btn3 = '<button id="cancel_waypoint" class="map_context_button">Cancel Waypoint Beacon</button>'
			var popup = L.popup(e.latlng, {
				"closeButton":false,
				"class" : "map_context_popup"
			})
			.setContent(btn1 + btn2 + generate_waypoint_period_buttons() + generate_waypoint_color_buttons() + HR + btn3)
			.openOn(this.map);  
			
			generate_waypoint_period_button_events();
			generate_waypoint_color_button_events();
			
			// We bind the click event here in the same scope as we add the Popup
			document.getElementById('place_waypoint').addEventListener('click', function(e){
				console.log("place_marker",lat,lon);
				Waypoint.send(lat,lon);
				popup.close();
			});
			
			// We bind the click event here in the same scope as we add the Popup
			document.getElementById('msg_waypoint').addEventListener('click', function(e){
				console.log("place_marker",lat,lon);
				let msg = prompt("Waypoint Message", retrive_waypoint_message());
				if (msg != ""){Cookies.set('waypoint_msg',msg, {expires:COOKIE_EXP});}
				Waypoint.send(lat,lon, msg);
				popup.close();
			});	
			
			document.getElementById('cancel_waypoint').addEventListener('click', function(e){
				console.log("cancel waypoint");
				messenger.postMessage(["waypoint",{'src':state.my_addr, 'dst':0, 'lat':0, 'lon':0, 'msg': '','color':'','cancel':true, 'period':retrive_waypoint_period()}]);
				Waypoint.update(state.my_addr, 0, 0, "", null, null, function(w){
					if (w.addr in waypoints){
						waypoints[w.addr].remove();
						delete waypoints[w.addr];
					}
				});
				popup.close();
			});
			
		});	
		
	}
	
	static tooltip_text(w,user){
		return `${user.callsign} ${Util.iso_to_time(w.time)}<br>${w.msg}`
	}
	
	static loaded_cb(wps){
		//console.log('WaypointView.loaded_cb',wps);
		for (const i in wps) {
			let w = wps[i];
			User.get(w.addr, function(user){
				waypoints[w.addr] = L.marker(L.latLng(w.lat, w.lon), {icon: WaypointView.icon()}).addTo(map);
				waypoints[w.addr].bindTooltip(
					WaypointView.tooltip_text(w,user),
					{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
				);
				if (waypoints[w.addr].getIcon() !== undefined){
					waypoints[w.addr].setIcon(WaypointView.icon(w.color));
					//waypoints[w.addr]._icon.style.fill = w.color;
				}
				if (w.addr == state.my_addr){
					messenger.postMessage(["waypoint",{'src':state.my_addr, 'dst':0, 'lat':w.lat, 'lon':w.lon, 'msg': w.msg,'color':w.color,'cancel':false, 'period':retrive_waypoint_period()}]);
				}
				WaypointView.update_span(w.addr, 'WAYPOINT ' + state.get_distance(waypoints[w.addr]) + 'm');
			});
		}
		load.set('waypoint_db');		
	}

	static icon(color='green'){
		const class_name = "waypoint_icon";
		var to_return = L.divIcon({
			html: Icon.waypoint_chevron(color),
			className: class_name,
			iconSize: [26, 26],
			iconAnchor: [10, 26]
		});
		return to_return
	}
	
	static update_span(addr, new_text){
		let e = $('#'+addr+'.user-entry-waypoint');
		e.html(new_text);
	}
	
	received_cb(data, timestamp){
		console.log('WaypointView.received_cb',data, timestamp);
		if (data.src == state.my_addr){
			WaypointView.update_span(data.src, 'WAYPOINT ' + state.get_distance(waypoints[data.src]) + 'm');
			return;
		}
		
		if (data.cancel === true) { //delete the waypoint
			console.log('delete waypoint from ' + data.src);
			Waypoint.update(data.src, 0, 0, "", null, "", function(w){
				if (data.src in waypoints){
					waypoints[data.src].remove();
					delete waypoints[data.src];
				}
			});
			return;
		}
		
		Waypoint.update(data.src, data.lat, data.lon, data.msg, timestamp, data.color, function(w, changed){
			//console.log(w);
			User.get(w.addr, function(user){
				//console.log(user);
				let txt = WaypointView.tooltip_text(w,user);
				let c = [w.lat,w.lon];
				if (w.addr in waypoints){
					waypoints[w.addr].setLatLng(c);
					waypoints[w.addr]._tooltip._content = txt;
					waypoints[w.addr].setIcon(WaypointView.icon(w.color));
				} else {
					const i = Object.keys(waypoints).length+1;
					waypoints[w.addr] = L.marker(c, {icon: WaypointView.icon()}).addTo(map);
					waypoints[w.addr].setIcon(WaypointView.icon(w.color));
					waypoints[w.addr].bindTooltip(txt,{permanent: false, direction: 'right', className: 'leaflet-tooltip'});	
				}	
				
				WaypointView.update_span(w.addr, 'WAYPOINT ' + state.get_distance(waypoints[w.addr]) + 'm');
				
				//need to make this popup more conditional
				if ((changed == true) && (c[0] != 0) && (c[1] != 0)){
					var popup = L.popup({className: "leaflet-tooltip"}).setLatLng(c).setContent('<p>'+txt+'</p>').openOn(map);
					setTimeout(function(){popup.close()}, 1500);
				}
				View.set_tab_pending("map");
			});
		});
	}
	
	static clear() {
		try {
			Waypoint.clear(); //delete all messages from the db
			for (const [k, w] of Object.entries(waypoints)) {
				w.remove();
			}
		} catch(error){
			console.log(error);
		}
		return false;
	}
	
}

////////////////////////////////////////////////////////////////
////////////////////Database Operations/////////////////////////
////////////////////////////////////////////////////////////////




function waypoint_db_init(waypoint_db_cb=Waypoint.load) {
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
			keyPath: "addr",
			autoIncrement: true,
		});

		// Define what data items the objectStore will contain
		objectStore.createIndex("addr", "addr", {unique: false });
		objectStore.createIndex("lat", "lat", 	{unique: false });
		objectStore.createIndex("lon", "lon", 	{unique: false });
		objectStore.createIndex("msg", "msg", 	{unique: false });
		objectStore.createIndex("time", "time", 		{unique: false });
		objectStore.createIndex("color", "color", 		{unique: false });

		console.log("Database setup complete");
	});
	
}