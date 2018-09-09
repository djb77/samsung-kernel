MELFAS FTS Touchscreen

Required properties:
- compatible: must be "stm,fts_touch"
- reg: I2C slave address of the chip (0x48)
- interrupt-parent: interrupt controller to which the chip is connected
- interrupts: interrupt to which the chip is connected

Example:
	hsi2c_23: hsi2c@108E0000 {
		status = "okay";
		samsung,reset-before-trans;
		touchscreen@49 {
			compatible = "stm,fts_touch";
			reg = <0x49>;
			pinctrl-names = "on_state", "off_state";
			pinctrl-0 = <&attn_irq>;
			pinctrl-1 = <&attn_input>;
			stm,tspid_gpio = <&gpe6 5 0>;
			stm,irq_gpio = <&gpa1 0 0>;
			stm,irq_type = <8200>;
			stm,num_lines = <32 16>;		/* rx tx */
			stm,max_coords = <1439 2959>;	/* x y */
			stm,grip_area = <512>;
			stm,regulator_dvdd = "vdd3";
			stm,regulator_avdd = "vdd5";
			stm,project_name = "Dream2", "G955";
			stm,firmware_name = "tsp_stm/fts8cd56_dream2.fw", "tsp_stm/fts8cd56_dream2.fw";
			stm,support_gesture = <1>;
		};
	};


