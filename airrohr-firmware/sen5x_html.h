
/*****************************************************************
 * add html helper functions SEN5X                       *
 *****************************************************************/
//#include <Arduino.h>
//#include "./utils.h"


static String form_select_mode_SEN5PM()
{
	String s_select1 = F(" selected='selected'");
	String s1 = F("<tr>"
				 "<td>" "&nbsp;&nbsp;- Emulate" ":&nbsp;</td>"
				 "<td>"
				 "<select id='current_lang' name='current_lang'>"
				 "<option value='SEN55'>SEN55</option>"
				 "<option value='SEN50'>SEN50</option>"
				 "<option value='SPS30'>SPS30</option>"
				 "</select>"
				 "</td>"
				 "</tr>");

	s1.replace("'" + String(cfg::sen5x_sym_pm) + "'>", "'" + String(cfg::sen5x_sym_pm) + "'" + s_select1 + ">");
	return s1;
	// page_content += s1;
}