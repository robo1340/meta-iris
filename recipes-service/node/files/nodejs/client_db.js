
let msg_db;
let user_db;
let settings_db;

var socket;

var location_watch_id = null;
var notifications = false;

function do_nothing(data) {return;}

////////////////////////////////////////////////////////////////
////////////////////Database Operations/////////////////////////
////////////////////////////////////////////////////////////////

class User {
	
	static load(){
		return user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
	}

	static add(addr, callsign=null, lat=null, lon=null, type=null){
		if (user_db === undefined){return}

		var last_activity_time = Util.get_iso_timestamp(); //"2011-12-19T15:28:46.493Z"
		var last_location_beacon_time = null;
		if ((lat !== null) && (lon !== null)){
			last_location_beacon_time = last_activity_time
		}
		
		const transaction = user_db.transaction(["notes_os"], "readwrite");
		const objectStore = transaction.objectStore("notes_os");
		const addRequest = objectStore.add({
			addr:addr, 
			callsign:callsign,
			lat:lat, lon:lon, 
			last_location_beacon_time:last_location_beacon_time, 
			last_activity_time:last_activity_time,
			type:type
		});		
	}

	static get(addr, cb, failed_cb=null) {
		if (user_db === undefined) {cb(null);}
		if (typeof addr === "string" || addr instanceof String) {
			addr = parseInt(addr,10);
		}
		const read_request = User.load().get(addr);
		read_request.onsuccess = function() {
			if (read_request.result === undefined) {
				if (failed_cb !== null){
					failed_cb();
				}
			}
			return cb(read_request.result);
		};	
	}

	static update(addr, callsign=null, lat=null, lon=null, timestamp=null, type=null, complete_cb=do_nothing){ 
		if (user_db === undefined){return;}
		if (timestamp == null){timestamp = Util.get_iso_timestamp();}
		
		const read_request = User.load().get(addr);
		read_request.onsuccess = function() {
			if (read_request.result === undefined){
				if ((lat !== null) && (lon !== null)){
					User.add(addr, callsign, lat, lon, type);
				} else {
					User.add(addr, callsign, null, null, type);
				}
			} else { //update user
				//console.log(read_request.result)
				var last_activity_time = timestamp; //"2011-12-19T15:28:46.493Z"
				var last_location_beacon_time = null;
				var new_lat = read_request.result.lat;
				var new_lon = read_request.result.lon;
				var new_callsign = read_request.result.callsign;
				if ((lat !== null) && (lon !== null)){
					last_location_beacon_time = last_activity_time;
					new_lat = lat;
					new_lon = lon;
				}
				if (callsign != null){
					new_callsign = callsign;
				}
				
				const transaction = user_db.transaction(["notes_os"], "readwrite");
				const objectStore = transaction.objectStore("notes_os");
				const updated_user = {
					addr:addr, 
					callsign:new_callsign,
					lat:new_lat, lon:new_lon, 
					last_location_beacon_time:last_location_beacon_time, 
					last_activity_time:last_activity_time,
					type:type
				};
				const addRequest = objectStore.put(updated_user);
				complete_cb(updated_user);
			}
		};	
	}
	
	static load_users(finished_cb=do_nothing) {
		if (user_db === undefined){return;}
		const objectStore = user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
		var users = objectStore.getAll();
		users.onsuccess = function() {
			finished_cb(users.result);
		};
	}

	static clear(finished_cb=do_nothing) {
		if (user_db === undefined){return;}	
		const objectStore = user_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
		objectStore.clear();
		finished_cb();
	}

	static remove(addr, finished_cb=do_nothing) {
		if (user_db === undefined){return;}	
		User.load().delete(addr); 
		finished_cb();
	}
	
	static user_db_cb() {
		function users_loaded_cb(users){
			//console.log(users)
			setTimeout(function(){
				let cnt = 0; //variable used to set the user color and placement in list
				
				for (const i in users){ //add me
					if (users[i].addr == state.my_addr){ 
						UserView.load_user(users[i]);
					}
				}				
				for (const i in users) {
					if (users[i].addr == state.my_addr){ continue;}			
					else {
						UserView.load_user(users[i]);
					}
				}
				load.set('user_list_loaded');
				
			}, LOAD_WAIT_MS);
			load.set('users_db');
		}
		User.update(0,'Iris');
		User.load_users(users_loaded_cb);
	}

}

class Message {
	
	static add(addr, msg, finished_cb=null) {
		//console.log("Message.add");
		if (msg_db === undefined){
			console.log("Message.add() failed msg_db is undefined");
			return
		}
		//console.log(msg);
		const transaction = msg_db.transaction(["notes_os"], "readwrite");
		const objectStore = transaction.objectStore("notes_os");
		const req = objectStore.add({addr:addr, msg:msg, time:Util.get_iso_timestamp()});	
		if (finished_cb !== null){
			req.onsuccess = finished_cb;
		}
	}
	
	static load(finished_cb=do_nothing) {
		if (msg_db !== undefined){
			const transaction = msg_db.transaction(["notes_os"], "readwrite");
			const objectStore = transaction.objectStore("notes_os");
			var messages = objectStore.getAll();
			messages.onsuccess = function() {
				finished_cb(messages.result);
			};
		}
	}
	
	static get(id, cb) {
		if (msg_db === undefined) {cb(null);}
		const read_request = msg_db.transaction(["notes_os"], "readwrite").objectStore("notes_os").get(id);
		read_request.onsuccess = function() {
			if (read_request.result === undefined) {return false;}
			return cb(read_request.result);
		};	
	}
	
	static update(id, msg, complete_cb=do_nothing){ 
		//console.log('Message.update',id,msg);
		if (msg_db === undefined){return;}
		const req = msg_db.transaction(["notes_os"], "readwrite").objectStore("notes_os").get(id);
		req.onsuccess = function() {
			if (req.result === undefined){
				return;
			} else { //update message	
				const transaction = msg_db.transaction(["notes_os"], "readwrite");
				const objectStore = transaction.objectStore("notes_os");
				const updated_msg = {
					id:req.result.id,
					addr:req.result.addr, 
					msg : msg,
					time:req.result.time
				};
				const addRequest = objectStore.put(updated_msg);
				complete_cb(updated_msg);
			}
		};	
	}

	static clear(finished_cb=do_nothing) {
		if (msg_db === undefined){return;}
		const objectStore = msg_db.transaction(["notes_os"], "readwrite").objectStore("notes_os");
		objectStore.clear();
		finished_cb();
	}
	
	static msg_db_cb() {
		function load_complete_cb(messages) {
			//console.log(messages);
			for (const i in messages) {
				MessageView.add(messages[i].addr, messages[i].msg);
			}
			load.set('msg_db');
		}
		Message.load(load_complete_cb);
	}

	
}

function setup_db(msg_db_cb=do_nothing,user_db_cb=do_nothing) {
	function setup_msg_db(cb){
		const msg_open_req  = window.indexedDB.open("msg_db", 1);
		msg_open_req.addEventListener("error", () =>
			console.error("msg_open_req failed to open"),
		);
		msg_open_req.addEventListener("success", () => {
			msg_db = msg_open_req.result;
			cb();
		});	

		msg_open_req.addEventListener("upgradeneeded", (e) => {
			console.log("Upgrade msg_db");
			msg_db = e.target.result;

			// Create an objectStore in our database to store notes and an auto-incrementing key
			// An objectStore is similar to a 'table' in a relational database
			const objectStore = msg_db.createObjectStore("notes_os", {
				keyPath: "id",
				autoIncrement: true,
			});

			// Define what data items the objectStore will contain
			objectStore.createIndex("addr", "addr", {unique: false });
			objectStore.createIndex("msg", "msg", 			{unique: false });
			objectStore.createIndex("time", "time", 		{unique: false });

			console.log("Database setup complete");
		});		
	}
	
	function setup_user_db(cb){
		const user_open_req = window.indexedDB.open("user_db", 1);
		user_open_req.addEventListener("error", () =>
			console.error("user_open_req failed to open"),
		);
		user_open_req.addEventListener("success", () => {
			user_db = user_open_req.result;
			cb();
		});
		
		user_open_req.addEventListener("upgradeneeded", (e) => {
			console.log("upgrade user_db");
			user_db = e.target.result;

			// Create an objectStore in our database to store notes and an auto-incrementing key
			// An objectStore is similar to a 'table' in a relational database
			const objectStore = user_db.createObjectStore("notes_os", {
				keyPath: "addr",
				//keyPath: "id",
				autoIncrement: true,
			});

			// Define what data items the objectStore will contain
			objectStore.createIndex("addr", "addr", {unique: false });
			objectStore.createIndex("callsign", "callsign", {unique: false });
			objectStore.createIndex("link_peer", "link_peer", {unique: false });
			objectStore.createIndex("lat", "lat", 			{unique: false });
			objectStore.createIndex("lon", "lon", 			{unique: false });
			objectStore.createIndex("last_location_beacon_time", "last_location_beacon_time", 	{unique: false });
			objectStore.createIndex("last_activity_time", "last_activity_time", 				{unique: false });
			objectStore.createIndex("type", "type", 				{unique: false });
			console.log("Database setup complete");
		});		
	}
	
	setup_user_db(function(){
		user_db_cb();
		setup_msg_db(msg_db_cb);
	});
	
}

////////////////////////////////////////////////////////////////
///////////////////////LOCATION RELATED/////////////////////////
////////////////////////////////////////////////////////////////

function position_received_cb(position){
	console.log('position_received_cb', position);
	if ((position.coords.latitude == 0) && (position.coords.longitude == 0)){return;}
	if (state === undefined){return;}
	
	state.my_lat = position.coords.latitude;
	state.my_lon = position.coords.longitude;
	Cookies.set('my_lat',state.my_lat, {expires:COOKIE_EXP});
	Cookies.set('my_lon',state.my_lon, {expires:COOKIE_EXP});
	messenger.postMessage(["my_location",{'src':state.my_addr,'dst':0,'lat':state.my_lat,'lon':state.my_lon,'period':state.location_period_s}]);
	
	if (load.complete() != true){return;} //don't do database things until the database is loaded
	
	//static update(addr, callsign=null, lat=null, lon=null, timestamp=null, type=null, complete_cb=do_nothing){ 
	User.update(state.my_addr, null, state.my_lat, state.my_lon, null, null, function(user) {
		if (Map.first_update == false){
			Map.first_update = true;
			Map.set_location(user.lat,user.lon);
			//UserView.load_user(user);	
		} else {
			let m = markers[state.my_addr]
			if (m !== undefined){
				m.setLatLng([state.my_lat, state.my_lon]);
			}
		}
	});
	
	UserView.update_all_distances();
	
}

const DEFAULT_COORDS = [37.54557, -97.26893];

function position_error_cb(error){
	if (Map.first_update == false){
		Map.set_location(DEFAULT_COORDS[0],DEFAULT_COORDS[1]);
		Map.first_update = true;
		var popup = L.popup(DEFAULT_COORDS, options={"class" : "map_context_popup"})
		.setContent("Location of Local Device Not Found, the map was set to a default location")
		.openOn(map);  
	}
	console.log(error);
}

function setup_location_watch(rx_cb=position_received_cb, error_cb=position_error_cb) {
	location_watch_id = navigator.geolocation.watchPosition(rx_cb, error_cb, {enableHighAccuracy:true, maximumAge:30000, timeout: 15000});
	//navigator.geolocation.clearWatch(watchID);
}

function setup_notifications(){
	if (!("Notification" in window)) {
		console.log("This browser does not support desktop notification");
	}
	else if (Notification.permission === "granted"){
		console.log("Notifications granted")
		notifications = true;
	}
	else if (Notification.permission !== "denied") {
		Notification.requestPermission().then((permission) => {
			notifications = true;
		});
	} else {
		console.log("Notifications denied")
	}
}