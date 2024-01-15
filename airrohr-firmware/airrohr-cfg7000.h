namespace cfg7
{
	bool s7000_has_gps = HAS_GPS;
	//char simm700_mode[3];
	char s7000_type[LEN_SIMM7000];
	char s7000_mode[LEN_SIMM7000];
	char s7000_apn[LEN_SIMM7000];

}

enum Config7000EntryType : unsigned short {
	Config7_Type_Bool,
	Config7_Type_UInt,
	Config7_Type_String,
};

struct Config7000ShapeEntry {
	enum Config7000EntryType cfg_type;
	unsigned short cfg_len;
	const char* _cfg_key;

	union {
		void* as_void;
		bool* as_bool;
		unsigned int* as_uint;
		char* as_str;
	} cfg_val;

	const __FlashStringHelper* cfg_key() const { return FPSTR(_cfg_key); }
};

enum ConfigShape7Id {
	Config7000_mode,
	Config7000_apn,
	Config7000_type,
	Config7000_has_gps,
};

static constexpr char CFG_KEY_7000_MODE[] PROGMEM = "s7000_mode";
static constexpr char CFG_KEY_7000_APN[] PROGMEM = "s7000_apn";
static constexpr char CFG_KEY_7000_TYPE[] PROGMEM = "s7000_type";
static constexpr char CFG_KEY_7000_HAS_GPS[] PROGMEM = "s7000_has_gps";


static constexpr Config7000ShapeEntry configShape7[] PROGMEM = {
	{ Config7_Type_String, sizeof(cfg7::s7000_mode)-1, CFG_KEY_7000_MODE, cfg7::s7000_mode },
	{ Config7_Type_String, sizeof(cfg7::s7000_apn)-1, CFG_KEY_7000_APN, cfg7::s7000_apn },
	{ Config7_Type_String, sizeof(cfg7::s7000_type)-1, CFG_KEY_7000_TYPE, cfg7::s7000_type },
	{ Config7_Type_Bool, 0, CFG_KEY_7000_HAS_GPS, &cfg7::s7000_has_gps },
};
