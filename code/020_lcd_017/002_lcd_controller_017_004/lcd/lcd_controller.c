

/* 向上：接收不同LCD的参数
 * 向下：使用这些参数设置对应的LCD控制器
*/

void led_controller_init(p_lcd_params plcdparams)
{
	/* 调用2440的lcd控制器的初始化函数  */
	led_controller.init(plcdparams);

}



