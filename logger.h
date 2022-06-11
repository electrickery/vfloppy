/* 
 * File:   logger.h
 * Author: fjkraan
 *
 * Created on January 5, 2013, 1:18 PM
 */

#ifndef LOGGER_H
#define	LOGGER_H

#ifdef	__cplusplus
extern "C" {
#endif

#define LOG_ERROR 0
#define LOG_WARN  1
#define LOG_INFO  2
#define LOG_DEBUG 3
#define LOG_TRACE 4

void msg(int level, const char *fmt, ...);
void setLogLevel(int level);
int  getLogLevel(); 


#ifdef	__cplusplus
}
#endif

#endif	/* LOGGER_H */

