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


/*****************************************************************
 * add html helper functions                                     *
 *****************************************************************/
static String form_select_mode7()		// mode SIMM7000
{
	String s_select = F(" selected='selected'");
	String s = F("<tr>"
				 "<td>" INTL_MODE ":&nbsp;</td>"
				 "<td>"
				 "<select id='s7000_mode' name='s7000_mode'>"
				 "<option value='1'>Mode 1</option>"
				 "<option value='2'>Mode 2</option>"
				 "<option value='3'>Mode 3</option>"
				 "<option value='4'>Mode 4</option>"
				 "</select>"
				 "</td>"
				 "</tr>");

	s.replace("'" + String(cfg7::s7000_mode) + "'>", "'" + String(cfg7::s7000_mode) + "'" + s_select + ">");
	return s;
}

static void add_form_input7(String &page_content, const ConfigShape7Id cfgid, const __FlashStringHelper *info, const int length)
{
	RESERVE_STRING(s, MED_STR);
	
	s = F("<tr>"
		  "<td title='[&lt;= {l}]'>{i}:&nbsp;</td>"
		  "<td style='width:{l}em'>"
		  "<input type='{t}' name='{n}' id='{n}' placeholder='{i}' value='{v}' maxlength='{l}'/>"
		  "</td></tr>");

	String t_value;
	Config7000ShapeEntry c;
	memcpy_P(&c, &configShape7[cfgid], sizeof(Config7000ShapeEntry));

	switch (c.cfg_type)
	{
	 case Config7_Type_UInt:
		t_value = String(*c.cfg_val.as_uint);
		s.replace("{t}", F("number"));
		break;

	// case Config7_Type_Time:
	// 	t_value = String((*c.cfg_val.as_uint) / 1000);
	// 	s.replace("{t}", F("number"));
	// 	break;

	 default:
	 		t_value = c.cfg_val.as_str;
	 		t_value.replace("'", "&#39;");
	 		s.replace("{t}", F("text"));
	}

	s.replace("{i}", info);
	s.replace("{n}", String(c.cfg_key()));
	s.replace("{v}", t_value);
	s.replace("{l}", String(length));
	page_content += s;
}

static String form_checkbox7(const ConfigShape7Id cfgid, const String &info, const bool linebreak)
{
	RESERVE_STRING(s, MED_STR);
	s = F("<label for='{n}'>"
		  "<input type='checkbox' name='{n}' value='1' id='{n}' {c}/>"
		  "<input type='hidden' name='{n}' value='0'/>"
		  "{i}</label><br/>");

	if (*configShape7[cfgid].cfg_val.as_bool)
	{
		s.replace("{c}", F(" checked='checked'"));
	}
	else
	{
		s.replace("{c}", emptyString);
	};

	s.replace("{i}", info);
	s.replace("{n}", String(configShape7[cfgid].cfg_key()));

	if (!linebreak)
	{
		s.replace("<br/>", emptyString);
	}

	return s;
}