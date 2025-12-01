#ifndef tlv_types_H_
#define tlv_types_H_

#define TYPE_END                0x00 ///<type used to indicate the end of a frame's data
#define TYPE_BEACON 			0xFF ///<type for beacons
#define TYPE_IPV4				0x04 ///<type for IPV4 datagrams
#define TYPE_IPV4_COMPRESSED 	0x14 ///<type for compressed IPV4 datagrams

#define TYPE_LINK_PEER_MANIFEST 0x11 ///<type for link peer manifest messages
#define TYPE_LINK_USER          0x12 ///<type for link user manifest messages
#define TYPE_LINK_USER_MANIFEST 0x13 ///<type for link user manifest messages

//#define TYPE_KNOWN(type) ((type==TYPE_BEACON) || (type==TYPE_IPV4) || (type==TYPE_IPV4_COMPRESSED))

typedef struct link_peer_manifest_t_DEFINITION link_peer_manifest_t; ///<object to hold info on available link peers
typedef struct link_user_notification_t_DEFINITION link_user_notification_t; ///<object to hold info on users that are connected to a link
typedef struct link_user_manifest_t_DEFINITION link_user_manifest_t; ///<object to hold info on users that are connected to a link
typedef struct link_user_t_DEFINITION link_user_t;					 ///<object to hold info on a user

struct __attribute__((__packed__)) link_user_t_DEFINITION {
	uint8_t ip[4]; ///<ip of the user
	char user_handle[32]; ///<the user's handle up to 31 characters long
};

struct __attribute__((__packed__)) link_peer_manifest_t_DEFINITION {
	uint8_t ip[4]; ///<ip of the originator of this message
	uint8_t link_peers[4][6]; ///<a list of up to 6 link peers ip's for this originator
	uint8_t ttl;
};

struct __attribute__((__packed__)) link_user_notification_t_DEFINITION {
	uint8_t ip[4]; ///<ip of the originator of this message
	link_user_t user; ///<the user this notification pertains to
	uint8_t user_action; ///<the action to be taken on this user
	uint8_t ttl;
};

struct __attribute__((__packed__)) link_user_manifest_t_DEFINITION {
	uint8_t ip[4]; ///<ip of the originator of this message
	link_user_t users[2]; ///<a list of up to two user's
	uint8_t ttl;
};





#endif
