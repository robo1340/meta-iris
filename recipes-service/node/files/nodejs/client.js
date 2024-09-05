

function add_saved_message(username, msg){
	addMessage(username, msg);
	add_unsaved_message(username, msg);
	
}

var lastGrey = false; //Used to alternate color of messages;

function add_unsaved_message(username, msg){
    var li = $("<li></li>",{
			"class": "message",
			"style": "background-color:" + (lastGrey ? "white" : "#eee") + ";list-style-type:none;",
			"text": username + ": " + msg
		});
    $("#list").append(li);
    lastGrey = !lastGrey;
    $('input[name=messageText]').val("");
    
    //scroll to bottom of div with scroll bar
    //setting position:fixed for this div is important
    //in adding a scroll bar to it
    $("#chatBox").scrollTop($("#chatBox")[0].scrollHeight);

	//$("#list").scrollTop($("#list").height());
	//$("#chatBox").scrollTop($("#chatBox").height());
	//$("div").scrollTop($("div").height());		
}

function connect(in_text) {
    console.log(in_text);
    in_text = in_text.split(" ");
    if (in_text[0].toLowerCase() != "connect" ||  in_text.length < 2) {
		add_unsaved_message("ERROR", "To connect, type 'connect' followed by a username");
        return;
    }
    my_username = in_text.slice(1).join(' ');
    console.log(in_text + "\t" + my_username);
	updateUser(my_username)
	Cookies.set('username',my_username, {expires:30})
	if (connected == true){
		socket.emit('newMessage', {'username': "Iris", 'message': "Username changed to " + my_username});
	} else {
		connected = true;
		setup_socket_callbacks(
			function(data){add_saved_message(data.username, data.message);},
			function(data){updateUser(data.username, data.coords[0], data.coords[1]);}
		);
		socket.emit('newMessage', {'username': "Iris", 'message': "User " + my_username +" has connected."});
	}
}

function onSend(e) {
	if (!connected) {
	    connect($("input[name=messageText]").val());
	} else {
		var msg = $('input[name=messageText]').val()
		var cmd = msg.toLowerCase();
		if (cmd.startsWith("connect")) {
			connect($("input[name=messageText]").val());
		}
		else if (cmd.startsWith("clear")){
			clear_messages(); //delete all messages from the db
			$("#list").empty();
		} else {
			console.log(msg)
			socket.emit('newMessage', {'username': my_username, 'message': msg});
		}
	}
	$("input[name=messageText]").val(''); //clear the text input
	return false;
}

function other_user_message_cb(data){
	console.log("other_user_message_cb");
	add_saved_message(data.username, data.message);
}

function other_user_location_cb(data){
	console.log("other_user_location_cb");
	updateUser(data.username, data.coords[0], data.coords[1]);
	updateUser(data.username, data.coords[0], data.coords[1]);
}


			
			//console.log(messages.result);

function load_complete_cb(messages) {
	for (i in messages) {
		add_unsaved_message(messages[i].username, messages[i].msg);
	}
	
	setTimeout(function(){
		if (my_username === undefined){
			my_username = 'unknown_user';
			add_unsaved_message('Iris', "Welcome! To get started type \"connect\" followed by your username");
		} else {
			connected = true;
			setup_socket_callbacks(other_user_message_cb,other_user_location_cb);
			add_unsaved_message('Iris', "Welcome! Back \"" + my_username + "\"");
		}	
	}, 500);
}

function msg_db_cb() {
	load_messages(load_complete_cb);
}

function user_db_cb() {
	return;
}

function toMap(e) {
	console.log('toMap()')
	window.location = '/map.html';
}

function toSettings(e){
	console.log('toSettings()')
}

$(document).ready(function() {
    $('button[name=sendMessage]').click(onSend);
	$('button[name=toMap]').click(toMap);
	$('button[name=toSettings]').click(toSettings);
    $('#messageInput').submit(onSend);
    $('#messageText').focus();
    
	my_username = Cookies.get('username');
	setup_db(msg_db_cb, user_db_cb);
	setup_location_watch();
	setup_notifications();
	setup_location_beacon();
	
//////////////////////
    //console.log(Date.now())
    
    socket = io.connect({query: "username=anonymous"});
    socket.emit('time', {'timestamp': Date.now()});
});


