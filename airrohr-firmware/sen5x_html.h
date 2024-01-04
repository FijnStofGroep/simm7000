
/*****************************************************************
 * add html helper functions SEN5X PMs, TS, NOx	(pin 1)	   		 *
 *****************************************************************/
String form_select_mode_SEN5PM()
{
	String s_select1 = F(" selected='selected'");
	String s1 = F("<tr>"
				  "<td>" INTL_SEN5X_EMP_PM "</td>"
				  "<td>"
				  "<select id='sen5x_sym_pm' name='sen5x_sym_pm'>"
				  //"<option value='SEN50'>SEN50</option>"
				  "<option value='SEN55'>SEN55</option>"
				  "<option value='SPS30'>SPS30</option>"
				  "</select>"
				  "</td>"
				  "</tr>");

	s1.replace("'" + String(cfg::sen5x_sym_pm) + "'>", "'" + String(cfg::sen5x_sym_pm) + "'" + s_select1 + ">");
	return s1;
}

/**************************************************************************
 * add html helper functions SEN5X temperature, humidity, (SCD30)CO2(NOx) *
 **************************************************************************/
String form_select_mode_SEN5TH()
{
	String s_select2 = F(" selected='selected'");
	String s2 = F("<tr>"
				  "<td>" INTL_SEN5X_EMP_TH "</td>"
				  "<td>"
				  "<select id='sen5x_sym_th' name='sen5x_sym_th'>"
				  "<option value='SEN55'>SEN55</option>"
				  "<option value='SCD30'>SCD30</option>"
				  "<option value='SHT35'>SHT35</option>"
				  "</select>"
				  "</td>"
				  "</tr>");

	s2.replace("'" + String(cfg::sen5x_sym_th) + "'>", "'" + String(cfg::sen5x_sym_th) + "'" + s_select2 + ">");
	return s2;
}