#ifndef _MCU_CONFIG_H    /* Guard against multiple inclusion */
#define _MCU_CONFIG_H

/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------
// DEFAULT CONFIGURATIONS
//--------------------------------------------------
/*================ ROW 1 =================*/
extern float CONFIG_H_IPH;
extern float CONFIG_H_RPM;
extern float CONFIG_POLES;

/*================ ROW 2 =================*/
extern float CONFIG_RPH;
extern float CONFIG_LD;
extern float CONFIG_LQ;
extern float CONFIG_V_CONSTANT;

/*================ ROW 3 =================*/
extern float CONFIG_ZERO_ANGLE;
extern float CONFIG_SENSOR_DIRECTION;
extern float CONFIG_ZERO_ANGLE_ESTIMATE;
extern float CONFIG_SENSOR_DIRECTION_ESTIMATE;

/*================ ROW 4 =================*/
extern float CONFIG_ROTATION_DIRECTION;
extern float CONFIG_CAN_BAUD;
extern float CONFIG_RPM_TO_KMPH;
extern float CONFIG_RPM_FAULT;
    
/*================ ROW 5 =================*/
extern float CONFIG_MOTOR_DERATE;
extern float CONFIG_MOTOR_TFAULT;
extern float CONFIG_MCU_DERATE;
extern float CONFIG_MCU_TFAULT;

/*================ ROW 6 =================*/
extern float CONFIG_VDC_MAX;
extern float CONFIG_IDC_DEFAULT;
extern float CONFIG_OV_FAULT;
extern float CONFIG_UV_FAULT;

/*================ ROW 7 =================*/
extern float CONFIG_IDC_PMAX;
extern float CONFIG_IDC_NMAX;
extern float CONFIG_IPH_FAULT;
extern float CONFIG_IDC_FAULT;

/*================ ROW 8 =================*/
extern float CONFIG_MOTOR_MODE;
extern float CONFIG_REVERSE_RPM;
extern float CONFIG_L_RPM;
extern float CONFIG_M_RPM;

/*================ ROW 9 =================*/
extern float CONFIG_THROTTLE_ZERO;
extern float CONFIG_THROTTLE_MAX;
extern float CONFIG_GEN_VLIMIT;
extern float CONFIG_AUTO_GEN;

/*================ ROW 10 =================*/
extern float CONFIG_RPM_KP;
extern float CONFIG_RPM_KI;
extern float CONFIG_L_ACCELERATION;
extern float CONFIG_M_ACCELERATION;

/*================ ROW 11 =================*/
extern float CONFIG_L_IDC;
extern float CONFIG_M_IDC;
extern float CONFIG_L_IPH;
extern float CONFIG_M_IPH;

/*================ ROW 12 =================*/
extern float CONFIG_BRAKE_IPH;
extern float CONFIG_BRAKE_ITIME;
extern float CONFIG_GEN_VMARGIN;
extern float CONFIG_NA1;

/*================ ROW 13 =================*/
extern float CONFIG_IPH_MAX;
extern float CONFIG_FMOTOR_MAX; 
extern float CONFIG_MOTOR_TMAX; 
extern float CONFIG_MCU_TMAX;

/*================ ROW 14 =================*/
extern float CONFIG_VMAX;
extern float CONFIG_VMIN; 
extern float CONFIG_IDC_MAX; 
extern float CONFIG_NA2;

/*================ ROW 15 =================*/
extern float CONFIG_RPM_KP_MIN; 
extern float CONFIG_RPM_KP_MAX; 
extern float CONFIG_RPM_KI_MIN; 
extern float CONFIG_RPM_KI_MAX; 

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
