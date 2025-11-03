/*
 * @file select_lang_html.h
 *
 * Written by R.Dieperink, Rolenco Leusden
 * Date: 2023-04-26
 *
 * Copyright (C) 2023
 *
 * Disable Firmware opties (FvD)
*/

namespace cfg 
{
	extern char current_lang[3];
	extern unsigned npm_heater_mode;
}

//*************************************************************************************************************
/*
  Input:
		value => pointer to HTTP webpage memory.
*/
static String form_submit(const String &value)
{
	String s = F("<tr>"
				 "<td>&nbsp;</td>"
				 "<td>"
				 "<input type='submit' name='submit' value='{v}' />"
				 "</td>"
				 "</tr>");

	s.replace("{v}", value);
	
	return s;
}

/// @brief 
/// @return 
static String form_select_lang()
{
	String s_select = F(" selected='selected'");
	String str = F("<tr>"
				 "<td>" INTL_LANGUAGE ":&nbsp;</td>"
				 "<td>"
				 "<select id='current_lang' name='current_lang'>"
				//  "<option value='BG'>Bulgarian (BG)</option>"
				//  "<option value='CN'>中文 (CN)</option>"
				//  "<option value='CZ'>Český (CZ)</option>"
				    "<option value='DE'>Deutsch (DE)</option>"
				//  "<option value='DK'>Dansk (DK)</option>"
				//  "<option value='EE'>Eesti keel (EE)</option>"
					"<option value='EN'>English (EN)</option>"
				//  "<option value='ES'>Español (ES)</option>"
				    "<option value='FR'>Français (FR)</option>"
				//  "<option value='GR'>Ελληνικά (GR)</option>"
				//  "<option value='IT'>Italiano (IT)</option>"
				//  "<option value='JP'>日本語 (JP)</option>"
				//  "<option value='LT'>Lietuvių kalba (LT)</option>"
				//  "<option value='LU'>Lëtzebuergesch (LU)</option>"
				//  "<option value='LV'>Latviešu valoda (LV)</option>"
				    "<option value='NL'>Nederlands (NL)</option>"
				//  "<option value='HU'>Magyar (HU)</option>"
				//  "<option value='PL'>Polski (PL)</option>"
				//  "<option value='PT'>Português (PT)</option>"
				//  "<option value='RO'>Română (RO)</option>"
				//  "<option value='RS'>Srpski (RS)</option>"
				//  "<option value='RU'>Русский (RU)</option>"
				//  "<option value='SI'>Slovenščina (SI)</option>"
				//  "<option value='SK'>Slovák (SK)</option>"
				//  "<option value='SE'>Svenska (SE)</option>"
				//  "<option value='TR'>Türkçe (TR)</option>"
				//  "<option value='UA'>український (UA)</option>"
				 "</select>"
				 "</td>"
				 "</tr>");

	str.replace("'" + String(cfg::current_lang) + "'>", "'" + String(cfg::current_lang) + "'" + s_select + ">");

	return str;
}

/**************************************************************************
 * Add html helper functions select: NextPM Heat mode					  *
 *   0     Sensor firmware setting (default).					  		  *
 *   1     Heater OFF (0%)            					  				  *
 *   2     Heater ON  (100%)           					  				  *
 *   3     Automatic heater regulation 			                          *
 **************************************************************************/
String form_select_NextPM_Heater_Mode()
{
	String s_select3 = F(" selected='selected'");
	String s3 = F("<tr>"
				  "<td>" INTL_NPM_HEATER_MODE ": </td>"
				  "<td>"
					"<select id='npm_heater_mode' name='npm_heater_mode'>"
					"<option value='0'>NONE</option>"
					"<option value='1'>OFF</option>"
					"<option value='2'>ON</option>"
					"<option value='3'>AUTO</option>"
					"<option value='4'>CONTROL</option>"
					"</select>"
				  "</td>"
				  "</tr>");

	s3.replace("'" + String(cfg::npm_heater_mode) + "'>", "'" + String(cfg::npm_heater_mode) + "'" + s_select3 + ">");
	return s3;
}
