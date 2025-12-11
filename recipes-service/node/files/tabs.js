
const COOKIE_EXP=30;
const LOAD_WAIT_MS=1000;
var state;
var map;
var waypoint_view;
var user_input;
var load;
var markers = {}; //a dictionary of location markers for other users

const COMMANDS = ["GET_MY_CALLSIGN","GET_MY_ADDR","GET_PEERS","GET_LINK_PEERS"]
const TLV_TYPES = ["ack","peer_info","location","message","waypoint","ping_request","ping_response","beacon"];
const META = ['rx_msg_cnt'];

class Util {

	static contains_object(list, obj) {
		for (const ele of list){
			if (ele == obj) {return true;}
		}
		return false;
	}

	static safe_execute(func, ...args){
		try {
			return func(args);
		} catch { 
			return null;
		}
	}
	
	static path_basename(path){
		return path.split('\\').pop().split('/').pop();
	}
	
	static get_iso_timestamp(date) {
		var date = new Date();
		return date.toLocaleString('sv').replace(' ', 'T');
	}
	
	static get_cookie(name, default_return=undefined, parser=null){
		try {
			var val = Cookies.get(name);
			//console.log(name, val);
			if (val === undefined){
				Cookies.set(name,default_return, {expires:COOKIE_EXP});
				return default_return;
			} else if (parser !== null){
				return parser(val);
			} else {
				return val;
			}
		} catch { 
			return default_return;
		}
	}
	
	static randint(max) {
	  return Math.floor(Math.random() * max);
	}
	
	static randfloat(min, max){
		return (Math.random() * (max-min) + min);
	}
	
	static hex(i,pad=2){
		return i.toString(16).padStart(pad,'0').toUpperCase();
	}
	
	static user_string(user){
		return user.callsign + "(" + Util.hex(user.addr,4) + ")"
	}
	
	static float_equals(f1, f2, delta=0.01){
		//console.log("float_equals("+f1+","+f2+")");
		//console.log(Math.abs(f1-f2));
		//console.log(delta + " " + (Math.abs(f1-f2) < delta));
		return (Math.abs(f1-f2) < delta);
	}
	
	//calculate the distance between two markers
	static get_distance(from, to){
		//console.log('get_distance');
		if ((from === undefined) || (to === undefined)) {return -1;}
		//console.log(from);
		//console.log(to);
		if ((from.lat !== undefined) && (from.lng !== undefined)){
			from = from;
		} else if (from.getLatLng !== undefined){
			from = from.getLatLng();
		} else {
			console.log('ERROR: get_distance',from);
			return -1;
		}
		if ((to.lat !== undefined) && (to.lng !== undefined)){
			to = to;
		} else if (to.getLatLng !== undefined){
			to = to.getLatLng();
		} else{
			console.log('ERROR: get_distance',to);
			return -1;
		}
		//console.log(from);
		//console.log(to);

		var to_return = from.distanceTo(to);
		if (to_return > 10000){return -1;} //return invalid if greater than 100km
		//console.log('get_distance()->' + to_return);
		return to_return.toFixed(0);
	}
	
	static iso_to_time(iso_str){
		try {
			return iso_str.split('T')[1].split('.')[0];
		} catch {return "";}
	}

	static iso_to_date(iso_str) {
		try {
			return iso_str.split('T')[0];
		} catch {return "";}
	}
	
}

class LoadStatus {
	constructor(interval_ms, complete_cb){
		let keys = ['users_db', 'msg_db', 'waypoint_db', 'my_callsign', 'my_addr'];
		this.load_status = {};
		this.complete_cb = complete_cb;
		for (const k in keys){
			this.load_status[keys[k]] = false;
		}
		this.load_complete = false;
		this.interval_ms = interval_ms;
		//console.log('LoadStatus',this.load_status);
		this.check = this.check.bind(this); //very important to ensure "this" is always this object
		this.load_status_interval = setInterval(this.check,this.interval_ms);
		return this;
	}
	
	set(key){
		this.load_status[key] = true;
	}
	
	complete(){
		return this.load_complete;
	}
	
	check(){
		var complete = true;
		for (const i in this.load_status){
			if (this.load_status[i] != true){complete=false;}
		}
		if (complete === true){
			this.load_complete = true;
			console.log('load complete!',this.load_status);
			clearInterval(this.load_status_interval);
			this.complete_cb();
		} else {
			//console.log('load not complete', this.load_status);
		}
	}
}

class State {
	constructor() {
		this.my_addr 		= Util.get_cookie('my_addr', undefined, parseInt);
		this.my_callsign 	= Util.get_cookie('my_callsign', undefined);
		this.my_lat 		= Util.get_cookie('my_lat', '37.5479', parseFloat);
		this.my_lon 		= Util.get_cookie('my_lon', '-97.2702', parseFloat);
		this.os_version		= Util.get_cookie('os_version', '-1.-1.-1', undefined);
		this.dst_addr		= 0;
		this.load_on_start 	= Util.get_cookie('load_on_start', "load_chat");
		this.list_users 	= Util.get_cookie('list_users', 'true')=='true';
		this.list_stations 	= Util.get_cookie('list_stations', 'true')=='true';
		this.ping_requests 	= {}; //dictionary of ping requests keys are user id's and values are the time of the last request
		this.msg_req_ack 	= Util.get_cookie('msg_req_ack','false')=='true';
		this.display_decimal_minute_second 	= Util.get_cookie('display_decimal_minute_second','true')=='true';
		this.messages_with_ack_pending = {};
		this.rx_msg_cnt = 0;
		this.randomize_locations = Util.get_cookie('randomize_locations','false')=='true';
		this.location_period_s = Util.get_cookie('location_period_s', 15, parseInt);
	}
	
	user_string(){
		return Util.user_string({'callsign':this.my_callsign,'addr':this.my_addr});
	}
	
	get_distance(from){
		return Util.get_distance(from, L.latLng(this.my_lat, this.my_lon));		
	}
}

class MessageView {

	static last_grey = false; //Used to alternate color of messages;

	static add_saved(addr, msg, unique_id=undefined, finished_cb=null){
		//console.log('add_saved',addr,msg);
		Message.add(addr, msg, finished_cb);
		MessageView.add(addr, msg, unique_id);
	}

	static add(addr, msg, unique_id=undefined){
		//console.log('add',addr,msg);
		User.get(addr, function(user){
			//console.log('add',user);
			var li = $("<li></li>",{
				"class": "message",
				"style": "background-color:" + (MessageView.last_grey ? "white" : "#eee") + ";list-style-type:none;",
				"text": Util.user_string(user) + ": " + msg,
				"unique_id" : unique_id
			}); 
			$("#list").append(li);
			MessageView.last_grey = !MessageView.last_grey;
			//$('input[name=messageText]').val("");
			
			//scroll to bottom of div with scroll bar
			//setting position:fixed for this div is important
			//in adding a scroll bar to it
			$("#list").scrollTop($("#list")[0].scrollHeight);


			//$("#list").scrollTop($("#list").height());
			//$("#chatBox").scrollTop($("#chatBox").height());
			//$("div").scrollTop($("div").height());			
		});
	}

	static clear() {
		try {
			Message.clear(); //delete all messages from the db
			$("#list").empty();
		}
		catch (error) {console.log(error);}
		return false;
	}

}

class UserView {
	static index = 1;
	static last_grey = false;

	static colors = {
		0 : "Crimson",
		1 : "DarkCyan",
		2 : "Chocolate",
		3 : "Chartreuse",
		4 : "Magenta",
		5 : "DarkOrange",
		6 : "ForestGreen",
		7 : "Gold",
		8 : "Aqua"
	}

	static color_by_id(id){
		return UserView.colors[(id)%Object.keys(UserView.colors).length];
	}

	static HTML_TRIANGLE_1 = `
				<svg
				  width="12"
				  height="24"
				  viewBox="0 0 100 100"
				  version="1.1"
				  preserveAspectRatio="none"
				  xmlns="http://www.w3.org/2000/svg"
				>
					<path d="M0 0 L50 100 L100 0 Z" fill="#7A8BE7"></path>
				</svg>`;

	static HTML_SQUARE_1 = `
				<svg
				  width="26"
				  height="26"
				  viewBox="0 0 100 100"
				  version="1.1"
				  preserveAspectRatio="none"
				  xmlns="http://www.w3.org/2000/svg"
				>
					<rect x="25" y="25" width="50" height="50" fill="blue" />
				</svg>`;

	static HTML_CIRCLE_1 = `
				<svg
				  width="18"
				  height="18"
				  viewBox="0 0 100 100"
				  version="1.1"
				  preserveAspectRatio="none"
				  xmlns="http://www.w3.org/2000/svg"
				>
					<circle r="45" cx="50" cy="50" fill="red" /></circle>
				</svg>`;
				
	static HTML_CIRCLE_2 = `
				<svg
				  width="14"
				  height="14"
				  viewBox="0 0 100 100"
				  version="1.1"
				  preserveAspectRatio="none"
				  xmlns="http://www.w3.org/2000/svg"
				>
					<circle r="45" cx="50" cy="50" fill="#7A8BE7" /></circle>
				</svg>`;
				
	static ICON_BY_USER_TYPE = {
		'station' : {'html' : UserView.HTML_TRIANGLE_1, 'size': [12, 24],'anchor' : [6,24]},
		'self' : {'html' : UserView.HTML_CIRCLE_1, 'size': [18, 18],'anchor' : [9, 9]},
		null : {'html' : UserView.HTML_CIRCLE_2, 'size': [14, 14],'anchor' : [7, 7]},
		undefined : {'html' : UserView.HTML_CIRCLE_2, 'size': [14, 14],'anchor' : [7, 7]},
		//undefined : {'html' : UserView.HTML_TRIANGLE_1, 'size': [24, 36],'anchor' : [12, 36]},
	};

	static user_icon_factory(id, type=null){
		//console.log(type);
		var class_name = "user_icon " + UserView.color_by_id(id);
		var to_return = L.divIcon({
			html: UserView.ICON_BY_USER_TYPE[type].html,
			className: class_name,
			iconSize: UserView.ICON_BY_USER_TYPE[type].size,
			iconAnchor: UserView.ICON_BY_USER_TYPE[type].anchor
		});	
		return to_return;
	}

	static circle_icon_factory(){
		const class_name = "circle_icon";
		var to_return = L.divIcon({
			html: `
				<svg
				  width="18"
				  height="18"
				  viewBox="0 0 100 100"
				  version="1.1"
				  preserveAspectRatio="none"
				  xmlns="http://www.w3.org/2000/svg"
				>
					<circle r="45" cx="50" cy="50" fill="red" /></circle>
				</svg>`,
			className: class_name,
			iconSize: [18, 18],
			iconAnchor: [9, 9]
		});	
		return to_return;
	}

	static load_user(user){
		//console.log('load_user', user, UserView.index);
		if (user.addr == 0){return;}
		//console.log('UserView.load_user',user);
		//const index = Object.keys(markers).length+1;
		var user_icon = UserView.user_icon_factory(UserView.index, user.type);
		if ((user.lat !== null) && (user.lon !== null) && (user.lat !== undefined) && (user.lon !== undefined) && (user.callsign != state.my_callsign)) {
			markers[user.addr] = L.marker([user.lat, user.lon], {icon: user_icon}).addTo(map);
			markers[user.addr].bindTooltip( UserView.tooltip_contents(user.callsign, user.last_location_beacon_time, user.lat, user.lon),
				{permanent: false, direction: 'right', className: 'leaflet-tooltip'}
			);
		}
		else if ((user.lat !== null) && (user.lon !== null) && (user.callsign == state.my_callsign)) {
			if (Map.first_update != true){
				Map.first_update = true;
				Map.set_location(user.lat, user.lon);
			}
		}
		
		UserView.add_user_to_list(user, UserView.index, true);	
		UserView.index += 1;
	}
	
	static add_user_to_list(user, index, stale=false){ 
		//console.log('UserView.add_user_to_list',user,index,stale);
		if (user.callsign === null){return;}
		
		//check for duplicates
		let e = $('#'+user.addr+'.user-entry');
		if (e.length > 0){
			console.log('duplicate user entry');
			return;
		}
		
		let me = user.callsign == state.my_callsign;
		var color = (me == false) ? UserView.color_by_id(index) : "ME";
		var time  =  ((stale == true)&&(me==false)) ?  "INACTIVE " : Util.iso_to_time(user.last_location_beacon_time);
		
		var li = $("<tr></tr>",{
				"class": "user-entry",
				"id" : user.addr,
				"style": "background-color:" + (UserView.last_grey ? "white" : "#eee") + ";list-style-type:none;",
			});
		var span_id = $("<td></td>",{
				"class": "user-entry-index " + color,
				"id" : user.addr,
				"text" : index + '.'
			});
		
		let display_name = (me==false) ? Util.user_string(user) : state.user_string() + '(Me)';
		
		var span_name = $("<td></td>",{
				"class": "user-entry-name",
				"id" : user.addr,
				"text" : display_name
			});
		var span_time = $("<td></td>",{
				"class": "user-entry-time",
				"id" : user.addr,
				"text" : time
			});
		var span_distance = $("<td></td>",{
				"class": "user-entry-distance",
				"id" : user.addr,
				"text" : state.get_distance(markers[user.addr])+'m'
			}); 
			
		let waypoint_text = "WAYPOINT " + state.get_distance(waypoints[user.addr])+'m';
		var span_waypoint = $("<td></td>",{
				"class": "user-entry-waypoint",
				"id" : user.addr,
				"text" : waypoint_text
			});
		var span_ping = $("<td></td>",{
				"class": "user-entry-ping",
				"id" : user.addr,
				"text" : 'PING'
			});
		
		li.append(span_id);
		li.append(span_name);
		li.append(span_distance);
		li.append(span_time);
		li.append(span_waypoint);
		if (me == false){
			li.append(span_ping);
		}
		$("#userList").append(li);
		UserView.last_grey = !UserView.last_grey;

		$("#"+user.addr+" .user-entry-name").click(function(e) {Map.go_to_user(e.currentTarget.id);});
		$("#"+user.addr+" .user-entry-time").click(function(e) {Map.go_to_user(e.currentTarget.id);});
		$("#"+user.addr+" .user-entry-waypoint").click(function(e) {Map.go_to_waypoint(e.currentTarget.id);});

		$("#"+user.addr+" .user-entry-ping").click(function(e) {
			if(e.currentTarget.id == state.my_addr){return;}
			let ping_id = Util.randint(256);
			let now = Date.now();
			if (state.ping_requests[e.currentTarget.id] !== undefined){
				if ((now - state.ping_requests[e.currentTarget.id]) < 1000){
					console.log('ping request suppressed, wait');
					return;
				}
			}
			state.ping_requests[e.currentTarget.id] = now;
			let req = {'src':state.my_addr, 'dst': parseInt(e.currentTarget.id), 'ping_id':ping_id};
			console.log('ping request', req);
			messenger.postMessage(["ping_request",req]);
			UserView.update_span_ping(e.currentTarget.id, 'REQ 0x' + Util.hex(ping_id,2));
		});
		
		//scroll to bottom of div with scroll bar
		//setting position:fixed for this div is important
		//in adding a scroll bar to it
		$("#userPane").scrollTop($("#userPane")[0].scrollHeight);
	}

	static clear_users_ui() {
		try {
			//User.clear(); //delete all user entries from the db
			$("#userList").empty();
			for (const [k, m] of Object.entries(markers)) {
				if (k == state.my_callsign){continue;}
				m.remove();
			}
		} catch(error){
			console.log(error);
		}
		return false;
	}
	
	static clear_inactive_users(){
		console.log('clear_inactive_users() currently incomplete');
		
		let items = document.querySelectorAll('#userList td');
		//items = items[0];
		//console.log(items);
		for (let i = items.length - 1; i >= 0; i--) {
			//for (const prop in items[i]){console.log(prop, items[i][prop]);}
			//console.log(items[i].className);
			if (items[i].className == 'user-entry-time'){
				if (items[i].textContent.includes('INACTIVE')){
					let addr = parseInt(items[i].id);
					console.log('deleting', addr);
					User.remove(addr, function(){
						//console.log('removed');
						let row = $('#'+addr+'.user-entry');
						row.remove()	
					});
					//console.log(items[i].attributes);
					//console.log(items[i].id);
				}
			}
		}	
	}
	
	static tooltip_contents(callsign, last_location_beacon_time, lat, lon){
		if (callsign == state.my_callsign){
			return `(Me) ${callsign}<br>${Map.coords_string(lat,lon)}`;
		} else {
			return `${callsign} ${Util.iso_to_time(last_location_beacon_time)}<br>${Map.coords_string(lat,lon)}`;
		}
		return to_return;
	}
	
	static update_span_ping(addr, new_text){
		let e = $('#'+addr+'.user-entry-ping');
		e.html(new_text);
		if (new_text.includes('REQ')){e.css("color","red");}
		else if (new_text.includes('RSP')){
			e.css("color","green");
			e.css("font-weight","bold");
			setTimeout(function(){
				e.css("font-weight","normal");
			}, 2000);
		}
	}

	static update_span_time(addr, new_text){
		//console.log('update_span_time',addr,new_text);
		let e = $('#'+addr+'.user-entry-time');
		e.html(new_text);	
		e.css("font-weight","bold");
		setTimeout(function(){
			e.css("font-weight","normal");
		}, 2000);
	}

	static update_span_distance(addr, new_text){
		let e = $('#'+addr+'.user-entry-distance');
		e.html(new_text);	
		e.css("font-weight","bold");
		setTimeout(function(){
			e.css("font-weight","normal");
		}, 2000);
	}

	static update_span_name(user){
		let e = $(`#${user.addr}.user-entry-name`);
		if (e === undefined){return;}
		let new_text = (user.addr!=state.my_addr) ? Util.user_string(user) : state.user_string() + '(Me)';
		e.html(new_text);
	}
	
	static update_all_distances(){
		console.log('UserView.update_all_distances()')
		if (user_db !== undefined){
			User.load_users(function(users){
				for (const user of users){
					UserView.update_span_distance(user.addr, state.get_distance(markers[user.addr]) + 'm');
				}
			});
		}
		if (waypoint_db !== undefined){
			Waypoint.load(function(waypoints){
				for (const w of waypoints){
					WaypointView.update_span(w.addr, 'WAYPOINT ' + state.get_distance(waypoints[w.addr]) + 'm');
				}
			});
		}
	}
	

}

class Map {
	static first_update = false;

	static set_location(lat, lon){
		console.log('Map.set_location',lat,lon);
		//console.log(lat_lon_arr)
		var lat_lon_arr = [lat, lon];
		map.setView(lat_lon_arr, 14);
		map.flyTo(lat_lon_arr, 14);	
		
		var my_location_marker = L.marker(lat_lon_arr, {icon: UserView.circle_icon_factory()}).addTo(map);
		my_location_marker.bindTooltip(UserView.tooltip_contents(state.my_callsign,null,lat,lon), {permanent: false});
		markers[state.my_addr] = my_location_marker;
	}
	
	static DD_to_DMS(deg) {
		var d = parseInt(deg);
		var md = Math.abs(deg - d) * 60;
		var m = parseInt(md);
		var sd = (md - m) * 60;
		return `${d}d${m}'${sd.toFixed(2)}"`; // Format as desired 
	}
	
	static coords_string(lat,lon,d=6){
		if (((typeof lat) != 'number')&&((typeof lon) != 'number')){return '';}
		let lat_str;
		let lon_str;
		if (state.display_decimal_minute_second == true){ //display in degree mode
			lat_str = Map.DD_to_DMS(lat);
			lon_str = Map.DD_to_DMS(lon);
		} else { //display in decimal mode
			lat_str = lat.toFixed(d);
			lon_str = lon.toFixed(d);
		}
		return `${lat_str}, ${lon_str}`;
	}
	
	static create_popup(contents, lat, lon, close_ms=1500){
		console.log('Map.create_popup',lat,lon);
		if ((lat==0) || (lon==0)){return;}
		var popup = L.popup({className: "leaflet-tooltip"}).setLatLng([lat,lon]).setContent(contents).openOn(map);
		setTimeout(function(){popup.close()}, close_ms);
		
	}
	
	static go_to_user(addr) {
		console.log('Map.go_to_user',addr);
		User.get(addr, function(user) {
			let contents = '<p>'+`${user.callsign}(${Util.hex(user.addr)})`+'<br />'+`(${Map.DD_to_DMS(user.lat)}, ${Map.DD_to_DMS(user.lon)})`+'</p>';
			Map.create_popup(contents, user.lat, user.lon);
		});
	}

	static go_to_waypoint(addr) {
		console.log('Map.go_to_waypoint', addr);
		Waypoint.get(addr, function(w){
			User.get(addr, function(user){
				let contents = '<p>'+`${user.callsign}(${Util.hex(user.addr)})`+'<br />'+w.msg+'<br />'+`(${Map.DD_to_DMS(w.lat)}, ${Map.DD_to_DMS(w.lon)})`+'</p>';
				Map.create_popup(contents, w.lat, w.lon);
			});
		});
	}
}

class View {
	static message_counter_color = document.getElementById('rx_msg_cnt').style.backgroundColor;


	//temporarily highlight a UI element by changing its background color
	static highlight(id, color, timeout_ms){
		let ele = document.getElementById(id);
		let prev_color = ele.style.backgroundColor;
		ele.style.backgroundColor = color;
		setTimeout(function(){
			ele.style.backgroundColor = prev_color;
		}, timeout_ms);
	}
	
	static highlight_element(ele, color, timeout_ms, return_color=null){
		let prev_color = ele.style.backgroundColor;
		if (return_color !== null){
			prev_color = return_color;
		}
		ele.style.backgroundColor = color;
		setTimeout(function(){
			ele.style.backgroundColor = prev_color;
		}, timeout_ms);		
	}

	static set_tab_buttons(disabled=false){
		document.getElementById("chat_button").disabled = disabled;
		document.getElementById("map_button").disabled = disabled;
		document.getElementById("settings_button").disabled = disabled;	
	}

	static list_map_elements(target){
		let load = true;
		//console.log('list_map_elements('+target+')');
		b = document.getElementById(target);
		if (b.className.includes('untoggled_button')) {
			b.className = b.className.replace(" untoggled_button", "");
			load = true;
		} else {
			b.className += " untoggled_button";
			load = false;
		}
		Cookies.set(target,load.toString(), {expires:COOKIE_EXP});
	}
	
	static set_tab_pending(tabname){
		try {
			if (document.getElementById(tabname).style.display == "none"){
				if (document.getElementById(tabname+"_button").className.includes("pending") != true){
					document.getElementById(tabname+"_button").className += " pending";
				}
			}
		} catch(error) {console.log(error);}
	}
	
	static open_tab(target) {
		var i, tabcontent, tablinks;

		// Get all elements with class="tabcontent" and hide them
		tabcontent = document.getElementsByClassName("tabcontent");
		for (i=0; i<tabcontent.length; i++) {
			tabcontent[i].style.display = "none";
		}

		// Get all elements with class="tablinks" and remove the class "active"
		tablinks = document.getElementsByClassName("tablinks");
		for (i=0; i<tablinks.length; i++) {
			if (tablinks[i].id == target){
				tablinks[i].className += " active";
				tablinks[i].className = tablinks[i].className.replace(" pending", "");
			} else {
				tablinks[i].className = tablinks[i].className.replace(" active", "");
			}
		}
		target = target.split('_')[0]
		document.getElementById(target).style.display = "block";
		if (target == "map"){
			map.invalidateSize()
		}
	}	
}

class UserInput {
	
	constructor(){
		$("input[name=load_on_start]").click(function(){
			//console.log($(this).val());
			Cookies.set('load_on_start',$(this).val(), {expires:COOKIE_EXP})
		});	
		
		document.getElementById("send_button").onclick = UserInput.send_msg;
		document.getElementById("clear_messages").onclick = MessageView.clear;
		document.getElementById("clear_users").onclick = this.clear_users;
		document.getElementById("clear_inactive_users").onclick = UserView.clear_inactive_users;
		document.getElementById("clear_waypoints").onclick = WaypointView.clear;
		document.getElementById("set_radio_config").onclick = this.set_radio_config;
		document.getElementById("set_hub_config").onclick = this.set_hub_config;
		document.getElementById("set_callsign").onclick = UserInput.set_callsign;
		document.getElementById("msg_req_ack").onclick = this.msg_req_ack;
		document.getElementById("display_decimal_minute_second").onclick = this.display_decimal_minute_second;
		document.getElementById("refresh_address_list").onclick = this.refresh_address_list;
		document.getElementById("destination_address").addEventListener('change', this.destination_address); 
		document.getElementById("randomize_locations").onclick = this.randomize_locations;
		document.getElementById("location_period_s").addEventListener("input", this.location_period_s);
	}
	
	clear_users(){
		console.log('clear_users');
		User.clear(function(){
			//reset UI
			UserView.clear_users_ui();
			UserView.index = 1;
			UserView.last_grey = false;
			
			//load myself back in
			User.update(state.my_addr, state.my_callsign, null, null, null, null, function(user){
				UserView.load_user(user);
				var my_location_marker = L.marker([state.my_lat,state.my_lon], {icon: UserView.circle_icon_factory()}).addTo(map);
				my_location_marker.bindTooltip(UserView.tooltip_contents(state.my_callsign,null,state.my_lat,state.my_lon), {permanent: false});
				markers[state.my_addr] = my_location_marker;
			});
		});
	}
	
	location_period_s(e){
		//console.log('location_period_s',e.target.value);
		let x = parseInt(e.target.value);
		if (Number.isNaN(x)){return;}
		state.location_period_s = x;
		Cookies.set('location_period_s',state.location_period_s, {expires:COOKIE_EXP});
		messenger.postMessage(["my_location",{'src':state.my_addr,'dst':0,'lat':state.my_lat,'lon':state.my_lon,'period':state.location_period_s}]);
	}
	
	destination_address(){ //change the destination address
		console.log('UserInput.destination_address');
		let addr = parseInt(this.value);
		if (Number.isNaN(addr)==false){
			state.dst_addr = addr;
		}
	}
	
	refresh_address_list(){
		let s = document.getElementById("destination_address");
		s.innerHTML = ""; //clear the Options
		s.add(new Option('Broadcast (0000)', 0));
		User.load_users(function(users){
			for (const i in users){
				if ((users[i].addr==0) || (users[i].addr == state.my_addr)){continue;}
				s.add(new Option(`${users[i].callsign} (${Util.hex(users[i].addr,4)})`, users[i].addr));
			}
		});		
	}
	
	msg_req_ack(){
		state.msg_req_ack = !state.msg_req_ack;
		//console.log('UserView.msg_req_ack()', state.msg_req_ack);
		let b = document.getElementById("msg_req_ack");
		b.checked = state.msg_req_ack;
		Cookies.set('msg_req_ack',state.msg_req_ack, {expires:COOKIE_EXP});
	}
	
	display_decimal_minute_second(){
		state.display_decimal_minute_second = !state.display_decimal_minute_second;
		let b = document.getElementById("display_decimal_minute_second");
		b.checked = state.display_decimal_minute_second;
		Cookies.set('display_decimal_minute_second', state.display_decimal_minute_second, {expires:COOKIE_EXP});
	}
	
	randomize_locations(){
		state.randomize_locations = !state.randomize_locations;
		let b = document.getElementById('randomize_locations');
		b.checked = state.randomize_locations;
		Cookies.set('randomize_locations', state.randomize_locations, {expires:COOKIE_EXP});
	}
	
	set_radio_config(){
		console.log('UserInput.set_radio_config()');
		var new_rc = {'radio':{},'snap_args':{}};
		
		new_rc.radio.modem 		= document.getElementById('radio_configs').value;
		new_rc.radio.general 	= document.getElementById('radio_general_configs').value;
		new_rc.radio.packet 	= document.getElementById('radio_packet_configs').value;
		new_rc.radio.preamble 	= document.getElementById('radio_preamble_configs').value;
		
		try {
			let x = parseInt(document.getElementById('payload').value);
			if (Number.isNaN(x) == false){
				new_rc.snap_args.payload = x;
			} else {
				View.highlight('payload','salmon',2500);
				throw new Error("payload value is not an integer");
			}
			x = parseInt(document.getElementById('block').value);
			if (Number.isNaN(x) == false){
				new_rc.snap_args.block = x;
			} else {
				View.highlight('block','salmon',2500);
				throw new Error("block value is not an integer");
			}
			new_rc.snap_args.disable_reed_solomon = document.getElementById('disable_reed_solomon').value;
			new_rc.snap_args.disable_convolutional  = document.getElementById('disable_convolutional').value;
			
			console.log('set_radio_config',new_rc);
			messenger.postMessage(["SET_RADIO_CONFIG",new_rc]);
			document.getElementById('set_radio_config').old_textContent = document.getElementById('set_radio_config').textContent;
			document.getElementById('set_radio_config').textContent = 'Change Pending';
			state.radio_config_set = true;
			setTimeout(function(){
				if (state.radio_config_set == true) { //request timed out
					console.log('SET_RADIO_CONFIG request timed out');
					state.radio_config_set = false;
					View.highlight("set_radio_config","salmon",2500);
					document.getElementById('set_radio_config').textContent = document.getElementById('set_radio_config').old_textContent;
				}
			}, 10000);
			
		}
		catch(error) {
			console.log(error);
			View.highlight('set_radio_config','salmon',2500);
		}
	}
	
	set_hub_config(){
		console.log('UserInput.set_hub_config()');
		var nc = {};
		
		nc.retransmit_wait_multiplier_s 	= parseFloat(document.getElementById('retransmit_wait_multiplier_s').value);
		nc.peer_timeout_s 	= document.getElementById('peer_timeout_s').value;
		nc.peer_info_tx_s 	= document.getElementById('peer_info_tx_s').value;
		nc.default_hops = document.getElementById('default_hops').value;
		
		try {
			nc.peer_timeout_s = parseInt(nc.peer_timeout_s);
			if (Number.isNaN(nc.peer_timeout_s) == true){
				View.highlight('peer_timeout_s','salmon',2500);
				throw new Error("peer_timeout_s value is not an integer");
			}
			
			nc.peer_info_tx_s = parseInt(nc.peer_info_tx_s);
			if (Number.isNaN(nc.peer_info_tx_s) == true){
				View.highlight('peer_info_tx_s','salmon',2500);
				throw new Error("peer_info_tx_s value is not an integer");
			}
			
			nc.default_hops = parseInt(nc.default_hops);
			if (Number.isNaN(nc.default_hops) == true){
				View.highlight('default_hops','salmon',2500);
				throw new Error("default_hops value is not an integer");
			} else if (nc.default_hops == 0){
				nc.default_hops = 1;
				document.getElementById('default_hops').value = nc.default_hops;
			}
			
			console.log('set_hub_config',nc);
			messenger.postMessage(["SET_HUB_CONFIG",nc]);
			document.getElementById('set_hub_config').old_textContent = document.getElementById('set_hub_config').textContent;
			document.getElementById('set_hub_config').textContent = 'Change Pending';
			state.hub_config_set = true;
			setTimeout(function(){
				if (state.hub_config_set == true) { //request timed out
					console.log('SET_HUB_CONFIG request timed out');
					state.hub_config_set = false;
					View.highlight("set_hub_config","salmon",2500);
					document.getElementById('set_hub_config').textContent = document.getElementById('set_hub_config').old_textContent;
				}
			}, 10000);
			
		}
		catch(error) {
			console.log(error);
			View.highlight('set_hub_config','salmon',2500);
		}
	}

	static send_msg() {
		try {
			let msg = prompt("Message:","");
			if ((msg !== null)&&(msg != "")){
				//console.log("UserInput.send_msg()");
				let unique_id = (state.msg_req_ack==true) ? Util.randint(65535) : null;
				messenger.postMessage(["message",{'src': state.my_addr, 'dst':state.dst_addr, 'msg': msg, 'want_ack':state.msg_req_ack, 'unique_id':unique_id}]);
				MessageView.add_saved(state.my_addr, msg, unique_id, function(e){
					state.messages_with_ack_pending[unique_id] = {'msg_db_key':e.target.result,'unique_id':unique_id, 'time':Util.get_iso_timestamp()};
					//console.log('msg',state.messages_with_ack_pending[unique_id]);
				});			
			}
		}
		catch (error) {console.log(error);}
		return false; //very important to not get redirected
	}

	static set_callsign(){
		try {
			let new_callsign = prompt("Enter New Callsign","");
			if ((new_callsign !== null)&&(new_callsign != "")){
				var old = state.my_callsign;
				state.my_callsign = new_callsign;
				messenger.postMessage(["SET_MY_CALLSIGN",{'callsign':state.my_callsign}]);
				
				if (old != state.my_callsign) {
					//Object.defineProperty(markers, state.my_callsign, Object.getOwnPropertyDescriptor(markers, old));
					//delete markers[state.my_addr];
					User.update(state.my_addr, state.my_callsign, null, null, null, null, function(user){
						//User.load(user);
						UserView.update_span_name(user);
						markers[user.addr]._tooltip._content = UserView.tooltip_contents(user.callsign,null,user.lat,user.lon);
					});
					
				}
			}
			document.getElementById("callsign_label").innerHTML = "Callsign: " + state.my_callsign;
		}
		catch (error) {console.log(error);}
		return false;
	}
}

$(document).ready(function() {
	var ip = location.host;
	document.getElementById("instructions").innerHTML = "If you are in a captive portal browser the application will not work, please navigate to https://" + ip + " in chrome or firefox"
	
	View.set_tab_buttons(true);
	View.open_tab("splash");
});

class RadioUtil {
	static config_path_extract(path){
		const freq_re 	 = new RegExp("(\\d+)M", "gm");
		const bitrate_re = new RegExp("(\\d+)kbps", "gm");
		var freq = freq_re.exec(path);
		var bitrate = bitrate_re.exec(path);
		if ((freq === null) || (bitrate === null)){
			console.log("throwing out " + path);
			return null;
		}
		return {"freq":freq[1],"bitrate":bitrate[1],"path":path};
	}
	
}

//handlers for when message is received from the backend client process
class Handlers {
	constructor(initial_handlers) { //setup initial handlers, when data bases are opened call this.init()
		this.handlers = initial_handlers;
		this.init_complete = false;
		//console.log("Constructor of:", this.constructor.name);
	}

	static callsign(pay, timestamp){
		//console.log('Handlers.callsign',pay); 
		state.my_callsign = pay.callsign;
		state.my_addr = pay.addr;
		Cookies.set('my_callsign',state.my_callsign, {expires:COOKIE_EXP});
		Cookies.set('my_addr',state.my_addr, {expires:COOKIE_EXP});
		load.set('my_callsign');
		document.getElementById("callsign_label").innerHTML = `My Callsign: ${state.my_callsign}`;
		load.set('my_addr');
		document.getElementById("addr_label").innerHTML = `My Address: 0x${Util.hex(state.my_addr,4)}`
		User.update(state.my_addr, state.my_callsign);
	}
	
	static message(data, timestamp){
		//console.log("Handlers.message", data);
		if (data.src == state.my_addr){return;}
		MessageView.add_saved(data.src, data.msg);
		if (notifications == true){
			User.get(msg.src, function(user){
				const notification = new Notification(`${user.addr}: ${data.msg}`);
			});
		}
		View.set_tab_pending("chat");
	}
	
	static location(data, timestamp){
		//console.log("Handlers.location", data);
		if (data === null){return;}
		if (data.src === undefined){return;}
		if ((data.lat === undefined) || (data.lon===undefined)){return;}
		if (load.complete() != true) {console.log('load not complete yet'); return;}
		
		
		/////////////////////////////////////
		//add some error to the coordinates//
		/////////////////////////////////////
		if (state.randomize_locations == true){
			data.lat = data.lat+Util.randfloat(-0.01,0.01);
			data.lon = data.lon+Util.randfloat(-0.01,0.01);
		}
		
		if (data.src == state.my_addr){
			state.my_lat = data.lat;
			state.my_lon = data.lon;
			Cookies.set('my_lat',state.my_lat, {expires:COOKIE_EXP});
			Cookies.set('my_lon',state.my_lon, {expires:COOKIE_EXP});
			
			User.update(data.src, null, data.lat, data.lon, null, null, function(user){
				if (Map.first_update != true){
					Map.first_update = true;
					Map.set_location(user.lat, user.lon);
				}
			});			
		} else {
			User.update(data.src, null, data.lat, data.lon, null, null, function(user){
				if (user.addr in markers){
					markers[user.addr].setLatLng([user.lat, user.lon]);
					markers[user.addr]._tooltip._content = UserView.tooltip_contents(user.callsign, user.last_location_beacon_time, user.lat, user.lon);

					UserView.update_span_time(user.addr, Util.iso_to_time(user.last_location_beacon_time));
					UserView.update_span_distance(user.addr, state.get_distance(markers[user.addr]) + 'm');
				} else {
					UserView.load_user(user);
				}
			});			
		}
	}
	
	static get_peers(data, timestamp){
		//console.log("Handlers.get_peers", data);
		for (const key in data) {
			var p = data[key]
			User.update(p.addr, p.callsign);
		}
	}
	
	static get_link_peers(data){
		//console.log("Handlers.get_link_peers", data);
		for (const key in data){
			var p = data[key];
			User.update(p.addr);
		}
	}
	
	static ping_request_cb(msg){
		console.log('ping_request_cb',msg);
		if ((msg['dst'] === undefined) || (msg['src'] === undefined)){return;}
		if (msg['dst'] != state.my_callsign) {return;}
		console.log('Ping Request Received from ' + msg['src'] + ' id:' + msg['ping_id'])
		messenger.postMessage(["ping_response",{'src':state.my_callsign, 'dst': msg['src'], 'ping_id': msg['ping_id']}]);
	}

	static ping_response_cb(msg){
		console.log('ping_response_cb',msg);
		if ((msg['dst'] === undefined) || (msg['src'] === undefined)){return;}
		if (msg['dst'] != state.my_addr) {return;}
		if (state.ping_requests[msg.src] === undefined){
			console.log('unknown ping response received ' + msg);
			return;
		} else {
			let diff = Date.now() - state.ping_requests[msg.src];
			UserView.update_span_ping(msg['src'], 'RSP ' + diff + 'ms');
			state.ping_requests[msg.src] = undefined;
		}
	}
	
	static available_configs_cb(a){
		var radio_configs = [];
		for (const i in a.modem){
			var c = RadioUtil.config_path_extract(a.modem[i]);
			if (c === null){continue;}
			radio_configs.push(c);
		}
		if (radio_configs.length == 0){
			radio_configs = null;
		}
		else {
			//console.log(radio_configs);
			for (const i in radio_configs){
				let rs = document.getElementById('radio_configs');
				rs.add(new Option(`${radio_configs[i]["freq"]}M ${radio_configs[i]["bitrate"]}kbps`, radio_configs[i]["path"]));
			} 
		}
		
		for (const i in a.general){
			let rs = document.getElementById('radio_general_configs');
			rs.add(new Option(Util.path_basename(a.general[i]), a.general[i]));
		}
		
		for (const i in a.packet){
			let rs = document.getElementById('radio_packet_configs');
			rs.add(new Option(Util.path_basename(a.packet[i]), a.packet[i]));			
		}
		
		for (const i in a.preamble){
			let rs = document.getElementById('radio_preamble_configs');
			rs.add(new Option(Util.path_basename(a.preamble[i]), a.preamble[i]));			
		}		
	}
	
	static get_radio_config_cb(msg){
		function select_element(id, value) {    
			let element = document.getElementById(id);
			element.value = value;
		}
		
		//console.log('Handlers.get_radio_config_cb', msg);
		Handlers.available_configs_cb(msg.available);

		//current modem config
		let c = RadioUtil.config_path_extract(msg.selected.modem);
		let lbl = document.getElementById('current_radio_config');
		if (c !== null){
			lbl.innerHTML = `Current: ${c["freq"]}M ${c["bitrate"]}kbps`;
			select_element('radio_configs', c['path']);
		}
		//current other configs
		select_element('radio_general_configs',msg.selected.general);
		select_element('radio_packet_configs', msg.selected.packet);
		select_element('radio_preamble_configs',msg.selected.preamble);
		
		//snap arguments
		for (const arg in msg.snap_args){
			select_element(arg, msg.snap_args[arg]);
		}
		
		if (state.radio_config_set == true){
			state.radio_config_set = false;
			View.highlight("set_radio_config","lime",2500);
			document.getElementById('set_radio_config').textContent = document.getElementById('set_radio_config').old_textContent;
			user_input.set_hub_config();
		}
		
	}
	
	static peer_info_cb(msg, timestamp){
		//console.log('Handlers.peer_info_cb', msg);
		User.get(msg.addr, function(user){ //success callback
			if (user.callsign != msg.callsign){
				MessageView.add_saved(0, `User ${user.callsign}(${Util.hex(user.addr)}) changed callsign to ${msg.callsign}`);
				View.set_tab_pending("chat");
				User.update(user.addr, msg.callsign, null, null, null, null, function(user_u){
					UserView.update_span_name(user_u);
					UserView.update_span_time(user_u.addr, Util.iso_to_time(timestamp));
					markers[user_u.addr]._tooltip._content = UserView.tooltip_contents(user_u.callsign, user_u.last_location_beacon_time, user_u.lat, user_u.lon);
					Waypoint.get(user_u.addr, function(w){
						waypoints[w.addr]._tooltip._content = WaypointView.tooltip_text(w,user_u);
					});
				});
			}
			else if (msg.callsign != state.my_callsign){
				UserView.update_span_time(user.addr, Util.iso_to_time(timestamp));
			}
		},
		function() { //failed callback
			User.update(msg.addr, msg.callsign, null, null, null, null, function(user){
				UserView.load_user(user);
			});					
			
			
		
		});
		
	}
	
	static get_hub_config_cb(msg){
		console.log('Handlers.get_hub_config_cb',msg);
		
		for (const key in msg){
			let ele = document.getElementById(key);
			if ((ele !== undefined)&&(ele !== null)){
				ele.value = msg[key].toString();
			}
			
		}
		if (state.hub_config_set == true){
			state.hub_config_set = false;
			View.highlight("set_hub_config","lime",2500);
			document.getElementById('set_hub_config').textContent = document.getElementById('set_hub_config').old_textContent;
		}	
		
	}
	
	static set_hub_config_cb(msg){
		console.log('Handlers.set_hub_config_cb',msg);
		
	}
	
	static ack_cb(pay){
		//console.log('Handler.ack_cb',pay);
		var my_ack_req = state.messages_with_ack_pending[pay.ack_id];
		if (my_ack_req !== undefined){
			console.log('ack received for', my_ack_req);
			delete state.messages_with_ack_pending[pay.ack_id];
			let ADDENDUM = ' ACKED';
			Message.get(my_ack_req.msg_db_key, function(m){
				m.msg = m.msg + ADDENDUM;
				Message.update(my_ack_req.msg_db_key, m.msg, function(m_new){
					let unique_id = pay.ack_id.toString();
					let items = document.querySelectorAll('#list li');
					for (let i = items.length - 1; i >= 0; i--) {
						//for (const prop in items[i]){console.log(prop, items[i][prop]);}
						if ((items[i].attributes.unique_id !== undefined) && (items[i].attributes.unique_id !== null) && (items[i].attributes.unique_id.value == unique_id)){
							items[i].textContent = items[i].textContent + ADDENDUM;
							View.highlight_element(items[i], 'aqua', 10000);
							break;
						}
					}
				});
			});
		}
	}
	
	static rx_msg_cnt_cb(pay){
		//console.log('rx_msg_cnt_cb',pay);
		state.rx_msg_cnt = pay.rx_msg_cnt;
		let b = document.getElementById('rx_msg_cnt');
		b.textContent = `RX: ${state.rx_msg_cnt}`;
		View.highlight_element(b, 'lime', 250, View.message_counter_color);	
	}

	static get_os_version(pay){
		console.log('get_os_version',pay);
		state.os_version = pay;
		document.getElementById('os_version').textContent = state.os_version;
		Cookies.set('os_version',state.os_version, {expires:COOKIE_EXP});
	}
	
	init(){
		for (const topic of COMMANDS) {
			if (this.handlers[topic] === undefined){
				this.handlers[topic] = function(pay, timestamp){console.log(topic,pay);}
			}
		}
		
		for (const topic of TLV_TYPES){
			if (this.handlers[topic] === undefined){
				this.handlers[topic] = function(pay, timestamp){console.log(topic,pay);}
			}
		}
		
		this.handlers['rx_msg_cnt'] = Handlers.rx_msg_cnt_cb;
		
		this.handlers['peer_info'] = Handlers.peer_info_cb;
		this.handlers['message'] = Handlers.message;
		this.handlers['location'] = Handlers.location;
		this.handlers['waypoint'] = waypoint_view.received_cb;
		this.handlers['ack'] = Handlers.ack_cb;
		this.handlers['GET_PEERS'] = Handlers.get_peers;
		this.handlers['GET_LINK_PEERS'] = Handlers.get_link_peers;
		this.handlers['ping_request'] = Handlers.ping_request_cb;
		this.handlers['ping_response'] = Handlers.ping_response_cb;
		this.handlers['GET_RADIO_CONFIG'] = Handlers.get_radio_config_cb;
		this.handlers['SET_RADIO_CONFIG'] = Handlers.get_radio_config_cb;
		this.handlers['GET_HUB_CONFIG'] = Handlers.get_hub_config_cb;
		this.handlers['SET_HUB_CONFIG'] = Handlers.get_hub_config_cb;
		this.handlers['SET_MY_CALLSIGN'] = function(msg){console.log('SET_MY_CALLSIGN success',msg);}
		this.handlers['GET_OS_VERSION'] = Handlers.get_os_version;
		this.init_complete = true;
	}
	
	handle(topic, data, timestamp){
		//console.log('Handlers.handle from backend',topic,data,timestamp);
		const h = this.handlers[topic];
		if (h === undefined){
			if (this.init_complete == true){
				console.log("unknown header received from worker",topic,data);
			}
			return;			
		} else {
			h(data, timestamp);
		}
	}
	
}


var handler = new Handlers({
	"GET_MY_CALLSIGN" : Handlers.callsign
});


var messenger = new Worker("message_worker.js");
messenger.onmessage = function(e) {
	handler.handle(e.data[0],e.data[1],e.data[2]);
};

//triggered whenever the databases have completed loading
load = new LoadStatus(250,function(){
	setTimeout(function(){
		MessageView.add(0, "Welcome Back \"" + state.my_callsign + "\"");
		const notification = new Notification("Welcome Back: " + state.my_callsign);
	}, LOAD_WAIT_MS);
	handler.init();
	messenger.postMessage(["GET_RADIO_CONFIG",'']);
	messenger.postMessage(["GET_HUB_CONFIG",'']);
	if ((Map.first_update != true) && (state.my_lat !== undefined) && (state.my_lon !== undefined)){
		Map.first_update = true;
		Map.set_location(state.my_lat, state.my_lon);
	}
	UserView.update_all_distances();
});

function user_accepted() {
	messenger.postMessage(["init",{}]);
	messenger.postMessage(["start",{}]);
	
	state = new State();
	console.log('main()',state);
	
	messenger.postMessage(["GET_MY_CALLSIGN",'']);
	messenger.postMessage(["GET_OS_VERSION",'']);
	
	View.set_tab_buttons(false);
	map = L.map('themap');

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
		//attribution: '<a href="https://openmaptiles.org/">&copy; OpenMapTiles</a>, <a href="http://www.openstreetmap.org/copyright">&copy; OpenStreetMap</a> contributors',
		vectorTileLayerStyles: vectorTileStyling,
		maxZoom: 14,
		minZoom: 14,
		//getFeatureId: function (e) {return}
	};

	var tile_grid = L.vectorGrid.protobuf("/tiles/{z}/{x}/{y}.pbf", openmaptilesVectorTileOptions).addTo(map);
	
	//map.on('click', function(e) {});
	
	waypoint_view = new WaypointView(map);
	user_input = new UserInput();
	
	///end of map setup///////////////////////////
	
	//init the radio button settings
	document.getElementById(state.load_on_start).checked = true;
	if (state.load_on_start == "load_chat"){
		$('#chat_button').click();
		$('#messageText').focus();		
	}
	else {
		$('#map_button').click();
	}

	document.getElementById("msg_req_ack").checked = state.msg_req_ack;
	document.getElementById("display_decimal_minute_second").checked = state.display_decimal_minute_second;
	document.getElementById('randomize_locations').checked = state.randomize_locations;
	document.getElementById("location_period_s").value = state.location_period_s;
	document.getElementById('os_version').textContent = state.os_version;
	
	if (state.list_users == false){
		b = document.getElementById('list_users');
		b.className += " untoggled_button";
	}
	if (state.list_stations == false){
		b = document.getElementById('list_stations');
		b.className += " untoggled_button";
	}
	
	setup_notifications();
	
	setup_db(Message.msg_db_cb, User.user_db_cb);
	waypoint_db_init();
	setup_location_watch();
	
	return false; //why does this return false?
}
