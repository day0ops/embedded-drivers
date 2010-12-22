/* GPGGA/GPGLL/GPRMC Data Parser
 *
 * Vanya A. Sergeev - <vsergeev@gmail.com> - copyright 2010
 * please inform author of possible use, licensing is still being decided
 */

#define CUSTOM_STRING_FUNCTIONS

/* Interface Data Storage */
typedef struct _GPS_Data {
	char nmea_type;
	char utc_time[11];
	char latitude[10];
	char n_s_indicator;
	char longitude[10];
	char e_w_indicator;
	char altitude[6];
	char units_altitude;
	char speed[6];
	char course[6];
	char utc_date[7];
	char data_valid;
} GPS_Data;

/* Interface Data Analysis Functions */
void GPS_Data_Clear(GPS_Data *gps_data);
int GPS_NMEA_Extract_Next(char *nmea_data, int len, char *nmea_string, int max_len, int *new_offset);
int GPS_NMEA_Parse_Next(char *nmea_string, GPS_Data *gps_data);
int GPS_NMEA_Type(char *nmea_string);

/* Interface Parsing Functions */
int GPGGA_Parse(char *nmea_string, GPS_Data *gps_data);
int GPGLL_Parse(char *nmea_string, GPS_Data *gps_data);
int GPRMC_Parse(char *nmea_string, GPS_Data *gps_data);

#define GPS_DATA_GGA		(1<<0)
#define GPS_DATA_GLL		(1<<1)
#define GPS_DATA_RMC		(1<<2)

