/**
 * @Author: ToSeven
 * @Date:   2017-04-06T20:53:52+08:00
 * @Email:  byone.heng@gmail.com
 * @Last modified by:   ToSeven
 * @Last modified time: 2017-04-07T19:56:05+08:00
 * @License: MIT
 */

#include "bsp_common.h"

__weak void loop(void){};

int main(void)
{
	bsp_init();  //bsp层初始化
    dispatch_init();
    on_ready();
    while(1)
  	 {
        HCI_Process();
        dispatch();
			  loop();
    }
}
