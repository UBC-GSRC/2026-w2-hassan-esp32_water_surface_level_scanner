/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : VL53L5_Plane_Algo.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
 
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __VL53L5_Plane_Algo_H
#define __VL53L5_Plane_Algo_H
 
#ifdef __cplusplus
extern "C" {
#endif
 
/* Includes ------------------------------------------------------------------*/
// #include "stm32f4xx_hal.h"
#include "vl53l5cx_api.h"
#include <stdio.h>
#include "math.h"
 
 
 
#define Pi 3.14156
 
typedef struct
{
	double Ax;
	double By;
	double Cz;
	double Offset;
	double XZ_Angle;
	double YZ_Angle;
}PlaneEquation_t;
 
typedef struct
{
	double Xpos[64];
	double Ypos[64];
	double Zpos[64];
}XYZ_ZoneCoordinates_t;
 
uint8_t Plane_Detection8x8(XYZ_ZoneCoordinates_t *XYZ, PlaneEquation_t *PlaneEq, uint16_t Accuracy_mm);
uint8_t ConvertDist2XYZCoords8x8(VL53L5CX_ResultsData *ResultsData, XYZ_ZoneCoordinates_t *XYZ_ZoneCoordinates);
uint8_t PrintResults(VL53L5CX_ResultsData *ResultsData);
uint8_t PrintXYZCoords(XYZ_ZoneCoordinates_t *XYZ_ZoneCoordinates);
uint8_t ComputeSinCosTables(void);
 
 
#endif /* __VL53L5_Plane_Algo_H */