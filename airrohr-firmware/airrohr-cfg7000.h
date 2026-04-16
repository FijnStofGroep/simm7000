/*
 *   airrohr-cfg7000.h
 *
 *
*/

enum Config7000EntryType : unsigned short
{
	Config7_Type_Bool,
	Config7_Type_UInt,
	Config7_Type_String,
    Config7_Type_Password,
};

/// @brief 
struct Config7000ShapeEntry
{
	enum Config7000EntryType cfg_type;
	unsigned short cfg_len;
	const char* _cfg_key;

	union
    {
		void* as_void;
		bool* as_bool;
		unsigned int* as_uint;
		char* as_str;
	} cfg_val;

	const __FlashStringHelper* cfg_key() const { return FPSTR(_cfg_key); }
};

/// @brief sequence for configShape7[] table.
enum ConfigShape7Id 
{
    Config7000_lteapn = 0,
    Config7000_lteUser,
    Config7000_ltePass,

	Config7000_type,
	Config7000_mode_selection,
    Config7000_communication_type,

	Config7000_has_gps,
};

static constexpr char CFG_KEY_APN[] PROGMEM = "apn-ID";
static constexpr char CFG_KEY_USER[] PROGMEM = "user";
static constexpr char CFG_KEY_PASS[] PROGMEM = "pass";

static constexpr char CFG_KEY_TYPE[] PROGMEM = "sim_type";
static constexpr char CFG_KEY_MODE_TYPE[] PROGMEM = "mode_selection";
static constexpr char CFG_KEY_COMMUNICATION_TYPE[] PROGMEM = "communication_type";

static constexpr char CFG_KEY_HAS_GPS[] PROGMEM = "has_gps";


static constexpr Config7000ShapeEntry configShape7[] PROGMEM = 
{
    { Config7_Type_String, sizeof(cfg7::lteapn)-1, CFG_KEY_APN, cfg7::lteapn },
    { Config7_Type_String, sizeof(cfg7::lteUser)-1, CFG_KEY_USER, cfg7::lteUser },
    { Config7_Type_String, sizeof(cfg7::ltePass)-1, CFG_KEY_PASS, cfg7::ltePass },

	{ Config7_Type_UInt, 0, CFG_KEY_TYPE, &cfg7::sim_type },
	{ Config7_Type_UInt, 0, CFG_KEY_MODE_TYPE, &cfg7::mode_selection },
    { Config7_Type_UInt, 0, CFG_KEY_COMMUNICATION_TYPE, &cfg7::communication_type },

	{ Config7_Type_Bool, 0, CFG_KEY_HAS_GPS, &cfg7::s7000_has_gps },
};
