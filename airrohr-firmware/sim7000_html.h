/*
    sim7000_html.h




*/

#ifndef __sim7000_html_h
#define __sim7000_html_h

/*****************************************************************
 * add html helper functions     SIM7000 GSM module              *
 *****************************************************************/
String form_select_mode7()		// mode SIMM7000
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

/// @brief 
/// @param page_content 
/// @param cfgid 
/// @param info 
/// @param length 
void add_form_input7(String &page_content, const ConfigShape7Id cfgid, const __FlashStringHelper *info, const int length)
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

	 default:   // Config7_Type_String
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

/// @brief 
/// @param cfgid 
/// @param info 
/// @param linebreak 
/// @return 
String form_checkbox7(const ConfigShape7Id cfgid, const String &info, const bool linebreak)
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

#endif // __Ssim7000_html_h