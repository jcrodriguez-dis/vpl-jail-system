/**
 * @package:   Part of vpl-jail-system
 * @copyright: Copyright (C) 2014 Juan Carlos Rodríguez-del-Pino
 * @license:   GNU/GPL, see LICENSE.txt or http://www.gnu.org/licenses/gpl-3.0.html
 **/

#ifndef JAIL_LIMITS_H_
#define JAIL_LIMITS_H_

#define JAIL_FILENAME_SIZE_LIMIT 128
#define JAIL_PATH_SIZE_LIMIT 256
#define JAIL_MIN_PRISONER_UID 1000
#define JAIL_MAX_PRISONER_UID (64*1024-5900)
#define JAIL_HEADERS_SIZE_LIMIT (8*1024)
#define JAIL_SOCKET_TIMEOUT 6
#define JAIL_MONITORSTART_TIMEOUT 10
#define JAIL_SOCKET_REQUESTTIMEOUT 20
#define JAIL_HARVEST_TIMEOUT 20
#define JAIL_NET_BUFFER_SIZE (1000*1024)
#define JAIL_ACCEPT_WAIT 100


#endif /* JAIL_LIMITS_H_ */
