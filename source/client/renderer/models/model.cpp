//**************************************************************************
//**
//**	See jlquake.txt for copyright info.
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 3
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//**  included (gnu.txt) GNU General Public License for more details.
//**
//**************************************************************************

#include "../local.h"
#include "../../../common/Common.h"
#include "../../../common/common_defs.h"
#include "../../../common/strings.h"
#include "../../../common/command_buffer.h"
#include "../../../common/endian.h"

model_t* loadmodel;

// precalculated dot products for quantized angles
float r_avertexnormal_dots[ SHADEDOT_QUANT ][ 256 ] =
{
	{
		1.23,1.30,1.47,1.35,1.56,1.71,1.37,1.38,1.59,1.60,1.79,1.97,1.88,1.92,1.79,1.02,0.93,1.07,0.82,0.87,0.88,0.94,0.96,1.14,1.11,0.82,0.83,0.89,0.89,0.86,0.94,0.91,
		1.00,1.21,0.98,1.48,1.30,1.57,0.96,1.07,1.14,1.60,1.61,1.40,1.37,1.72,1.78,1.79,1.93,1.99,1.90,1.68,1.71,1.86,1.60,1.68,1.78,1.86,1.93,1.99,1.97,1.44,1.22,1.49,
		0.93,0.99,0.99,1.23,1.22,1.44,1.49,0.89,0.89,0.97,0.91,0.98,1.19,0.82,0.76,0.82,0.71,0.72,0.73,0.76,0.79,0.86,0.83,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,
		0.83,0.96,1.14,0.97,1.40,1.19,0.98,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.87,0.93,0.94,1.02,1.30,
		1.07,1.35,1.38,1.11,1.56,1.92,1.79,1.79,1.59,1.60,1.72,1.90,1.79,0.80,0.85,0.79,0.93,0.80,0.85,0.77,0.74,0.72,0.77,0.74,0.72,0.70,0.70,0.71,0.76,0.73,0.79,0.79,
		0.73,0.76,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.26,1.26,1.48,1.23,1.50,1.71,1.14,1.19,1.38,1.46,1.64,1.94,1.87,1.84,1.71,1.02,0.92,1.00,0.79,0.85,0.84,0.91,0.90,0.98,0.99,0.77,0.77,0.83,0.82,0.79,0.86,0.84,
		0.92,0.99,0.91,1.24,1.03,1.33,0.88,0.94,0.97,1.41,1.39,1.18,1.11,1.51,1.61,1.59,1.80,1.91,1.76,1.54,1.65,1.76,1.70,1.70,1.85,1.85,1.97,1.99,1.93,1.28,1.09,1.39,
		0.92,0.97,0.99,1.18,1.26,1.52,1.48,0.83,0.85,0.90,0.88,0.93,1.00,0.77,0.73,0.78,0.72,0.71,0.74,0.75,0.79,0.86,0.81,0.75,0.81,0.79,0.96,0.88,0.94,0.86,0.93,0.92,
		0.85,1.08,1.33,1.05,1.55,1.31,1.01,1.05,1.27,1.31,1.60,1.47,1.70,1.54,1.76,1.76,1.57,0.93,0.90,0.99,0.88,0.88,0.95,0.97,1.11,1.39,1.20,0.92,0.97,1.01,1.10,1.39,
		1.22,1.51,1.58,1.32,1.64,1.97,1.85,1.91,1.77,1.74,1.88,1.99,1.91,0.79,0.86,0.80,0.94,0.84,0.88,0.74,0.74,0.71,0.82,0.77,0.76,0.70,0.73,0.72,0.73,0.70,0.74,0.85,
		0.77,0.82,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.34,1.27,1.53,1.17,1.46,1.71,0.98,1.05,1.20,1.34,1.48,1.86,1.82,1.71,1.62,1.09,0.94,0.99,0.79,0.85,0.82,0.90,0.87,0.93,0.96,0.76,0.74,0.79,0.76,0.74,0.79,0.78,
		0.85,0.92,0.85,1.00,0.93,1.06,0.81,0.86,0.89,1.16,1.12,0.97,0.95,1.28,1.38,1.35,1.60,1.77,1.57,1.33,1.50,1.58,1.69,1.63,1.82,1.74,1.91,1.92,1.80,1.04,0.97,1.21,
		0.90,0.93,0.97,1.05,1.21,1.48,1.37,0.77,0.80,0.84,0.85,0.88,0.92,0.73,0.71,0.74,0.74,0.71,0.75,0.73,0.79,0.84,0.78,0.79,0.86,0.81,1.05,0.94,0.99,0.90,0.95,0.92,
		0.86,1.24,1.44,1.14,1.59,1.34,1.02,1.27,1.50,1.49,1.80,1.69,1.86,1.72,1.87,1.80,1.69,1.00,0.98,1.23,0.95,0.96,1.09,1.16,1.37,1.63,1.46,0.99,1.10,1.25,1.24,1.51,
		1.41,1.67,1.77,1.55,1.72,1.95,1.89,1.98,1.91,1.86,1.97,1.99,1.94,0.81,0.89,0.85,0.98,0.90,0.94,0.75,0.78,0.73,0.89,0.83,0.82,0.72,0.77,0.76,0.72,0.70,0.71,0.91,
		0.83,0.89,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.46,1.34,1.60,1.16,1.46,1.71,0.94,0.99,1.05,1.26,1.33,1.74,1.76,1.57,1.54,1.23,0.98,1.05,0.83,0.89,0.84,0.92,0.87,0.91,0.96,0.78,0.74,0.79,0.72,0.72,0.75,0.76,
		0.80,0.88,0.83,0.94,0.87,0.95,0.76,0.80,0.82,0.97,0.96,0.89,0.88,1.08,1.11,1.10,1.37,1.59,1.37,1.07,1.27,1.34,1.57,1.45,1.69,1.55,1.77,1.79,1.60,0.93,0.90,0.99,
		0.86,0.87,0.93,0.96,1.07,1.35,1.18,0.73,0.76,0.77,0.81,0.82,0.85,0.70,0.71,0.72,0.78,0.73,0.77,0.73,0.79,0.82,0.76,0.83,0.90,0.84,1.18,0.98,1.03,0.92,0.95,0.90,
		0.86,1.32,1.45,1.15,1.53,1.27,0.99,1.42,1.65,1.58,1.93,1.83,1.94,1.81,1.88,1.74,1.70,1.19,1.17,1.44,1.11,1.15,1.36,1.41,1.61,1.81,1.67,1.22,1.34,1.50,1.42,1.65,
		1.61,1.82,1.91,1.75,1.80,1.89,1.89,1.98,1.99,1.94,1.98,1.92,1.87,0.86,0.95,0.92,1.14,0.98,1.03,0.79,0.84,0.77,0.97,0.90,0.89,0.76,0.82,0.82,0.74,0.72,0.71,0.98,
		0.89,0.97,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.60,1.44,1.68,1.22,1.49,1.71,0.93,0.99,0.99,1.23,1.22,1.60,1.68,1.44,1.49,1.40,1.14,1.19,0.89,0.96,0.89,0.97,0.89,0.91,0.98,0.82,0.76,0.82,0.71,0.72,0.73,0.76,
		0.79,0.86,0.83,0.91,0.83,0.89,0.72,0.76,0.76,0.89,0.89,0.82,0.82,0.98,0.96,0.97,1.14,1.40,1.19,0.94,1.00,1.07,1.37,1.21,1.48,1.30,1.57,1.61,1.37,0.86,0.83,0.91,
		0.82,0.82,0.88,0.89,0.96,1.14,0.98,0.70,0.72,0.73,0.77,0.76,0.79,0.70,0.72,0.71,0.82,0.77,0.80,0.74,0.79,0.80,0.74,0.87,0.93,0.85,1.23,1.02,1.02,0.93,0.93,0.87,
		0.85,1.30,1.35,1.07,1.38,1.11,0.94,1.47,1.71,1.56,1.97,1.88,1.92,1.79,1.79,1.59,1.60,1.30,1.35,1.56,1.37,1.38,1.59,1.60,1.79,1.92,1.79,1.48,1.57,1.72,1.61,1.78,
		1.79,1.93,1.99,1.90,1.86,1.78,1.86,1.93,1.99,1.97,1.90,1.79,1.72,0.94,1.07,1.00,1.37,1.21,1.30,0.86,0.91,0.83,1.14,0.98,0.96,0.82,0.88,0.89,0.79,0.76,0.73,1.07,
		0.94,1.11,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.74,1.57,1.76,1.33,1.54,1.71,0.94,1.05,0.99,1.26,1.16,1.46,1.60,1.34,1.46,1.59,1.37,1.37,0.97,1.11,0.96,1.10,0.95,0.94,1.08,0.89,0.82,0.88,0.72,0.76,0.75,0.80,
		0.80,0.88,0.87,0.91,0.83,0.87,0.72,0.76,0.74,0.83,0.84,0.78,0.79,0.96,0.89,0.92,0.98,1.23,1.05,0.86,0.92,0.95,1.11,0.98,1.22,1.03,1.34,1.42,1.14,0.79,0.77,0.84,
		0.78,0.76,0.82,0.82,0.89,0.97,0.90,0.70,0.71,0.71,0.73,0.72,0.74,0.73,0.76,0.72,0.86,0.81,0.82,0.76,0.79,0.77,0.73,0.90,0.95,0.86,1.18,1.03,0.98,0.92,0.90,0.83,
		0.84,1.19,1.17,0.98,1.15,0.97,0.89,1.42,1.65,1.44,1.93,1.83,1.81,1.67,1.61,1.36,1.41,1.32,1.45,1.58,1.57,1.53,1.74,1.70,1.88,1.94,1.81,1.69,1.77,1.87,1.79,1.89,
		1.92,1.98,1.99,1.98,1.89,1.65,1.80,1.82,1.91,1.94,1.75,1.61,1.50,1.07,1.34,1.27,1.60,1.45,1.55,0.93,0.99,0.90,1.35,1.18,1.07,0.87,0.93,0.96,0.85,0.82,0.77,1.15,
		0.99,1.27,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.86,1.71,1.82,1.48,1.62,1.71,0.98,1.20,1.05,1.34,1.17,1.34,1.53,1.27,1.46,1.77,1.60,1.57,1.16,1.38,1.12,1.35,1.06,1.00,1.28,0.97,0.89,0.95,0.76,0.81,0.79,0.86,
		0.85,0.92,0.93,0.93,0.85,0.87,0.74,0.78,0.74,0.79,0.82,0.76,0.79,0.96,0.85,0.90,0.94,1.09,0.99,0.81,0.85,0.89,0.95,0.90,0.99,0.94,1.10,1.24,0.98,0.75,0.73,0.78,
		0.74,0.72,0.77,0.76,0.82,0.89,0.83,0.73,0.71,0.71,0.71,0.70,0.72,0.77,0.80,0.74,0.90,0.85,0.84,0.78,0.79,0.75,0.73,0.92,0.95,0.86,1.05,0.99,0.94,0.90,0.86,0.79,
		0.81,1.00,0.98,0.91,0.96,0.89,0.83,1.27,1.50,1.23,1.80,1.69,1.63,1.46,1.37,1.09,1.16,1.24,1.44,1.49,1.69,1.59,1.80,1.69,1.87,1.86,1.72,1.82,1.91,1.94,1.92,1.95,
		1.99,1.98,1.91,1.97,1.89,1.51,1.72,1.67,1.77,1.86,1.55,1.41,1.25,1.33,1.58,1.50,1.80,1.63,1.74,1.04,1.21,0.97,1.48,1.37,1.21,0.93,0.97,1.05,0.92,0.88,0.84,1.14,
		1.02,1.34,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.94,1.84,1.87,1.64,1.71,1.71,1.14,1.38,1.19,1.46,1.23,1.26,1.48,1.26,1.50,1.91,1.80,1.76,1.41,1.61,1.39,1.59,1.33,1.24,1.51,1.18,0.97,1.11,0.82,0.88,0.86,0.94,
		0.92,0.99,1.03,0.98,0.91,0.90,0.79,0.84,0.77,0.79,0.84,0.77,0.83,0.99,0.85,0.91,0.92,1.02,1.00,0.79,0.80,0.86,0.88,0.84,0.92,0.88,0.97,1.10,0.94,0.74,0.71,0.74,
		0.72,0.70,0.73,0.72,0.76,0.82,0.77,0.77,0.73,0.74,0.71,0.70,0.73,0.83,0.85,0.78,0.92,0.88,0.86,0.81,0.79,0.74,0.75,0.92,0.93,0.85,0.96,0.94,0.88,0.86,0.81,0.75,
		0.79,0.93,0.90,0.85,0.88,0.82,0.77,1.05,1.27,0.99,1.60,1.47,1.39,1.20,1.11,0.95,0.97,1.08,1.33,1.31,1.70,1.55,1.76,1.57,1.76,1.70,1.54,1.85,1.97,1.91,1.99,1.97,
		1.99,1.91,1.77,1.88,1.85,1.39,1.64,1.51,1.58,1.74,1.32,1.22,1.01,1.54,1.76,1.65,1.93,1.70,1.85,1.28,1.39,1.09,1.52,1.48,1.26,0.97,0.99,1.18,1.00,0.93,0.90,1.05,
		1.01,1.31,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.97,1.92,1.88,1.79,1.79,1.71,1.37,1.59,1.38,1.60,1.35,1.23,1.47,1.30,1.56,1.99,1.93,1.90,1.60,1.78,1.61,1.79,1.57,1.48,1.72,1.40,1.14,1.37,0.89,0.96,0.94,1.07,
		1.00,1.21,1.30,1.14,0.98,0.96,0.86,0.91,0.83,0.82,0.88,0.82,0.89,1.11,0.87,0.94,0.93,1.02,1.07,0.80,0.79,0.85,0.82,0.80,0.87,0.85,0.93,1.02,0.93,0.77,0.72,0.74,
		0.71,0.70,0.70,0.71,0.72,0.77,0.74,0.82,0.76,0.79,0.72,0.73,0.76,0.89,0.89,0.82,0.93,0.91,0.86,0.83,0.79,0.73,0.76,0.91,0.89,0.83,0.89,0.89,0.82,0.82,0.76,0.72,
		0.76,0.86,0.83,0.79,0.82,0.76,0.73,0.94,1.00,0.91,1.37,1.21,1.14,0.98,0.96,0.88,0.89,0.96,1.14,1.07,1.60,1.40,1.61,1.37,1.57,1.48,1.30,1.78,1.93,1.79,1.99,1.92,
		1.90,1.79,1.59,1.72,1.79,1.30,1.56,1.35,1.38,1.60,1.11,1.07,0.94,1.68,1.86,1.71,1.97,1.68,1.86,1.44,1.49,1.22,1.44,1.49,1.22,0.99,0.99,1.23,1.19,0.98,0.97,0.97,
		0.98,1.19,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.94,1.97,1.87,1.91,1.85,1.71,1.60,1.77,1.58,1.74,1.51,1.26,1.48,1.39,1.64,1.99,1.97,1.99,1.70,1.85,1.76,1.91,1.76,1.70,1.88,1.55,1.33,1.57,0.96,1.08,1.05,1.31,
		1.27,1.47,1.54,1.39,1.20,1.11,0.93,0.99,0.90,0.88,0.95,0.88,0.97,1.32,0.92,1.01,0.97,1.10,1.22,0.84,0.80,0.88,0.79,0.79,0.85,0.86,0.92,1.02,0.94,0.82,0.76,0.77,
		0.72,0.73,0.70,0.72,0.71,0.74,0.74,0.88,0.81,0.85,0.75,0.77,0.82,0.94,0.93,0.86,0.92,0.92,0.86,0.85,0.79,0.74,0.79,0.88,0.85,0.81,0.82,0.83,0.77,0.78,0.73,0.71,
		0.75,0.79,0.77,0.74,0.77,0.73,0.70,0.86,0.92,0.84,1.14,0.99,0.98,0.91,0.90,0.84,0.83,0.88,0.97,0.94,1.41,1.18,1.39,1.11,1.33,1.24,1.03,1.61,1.80,1.59,1.91,1.84,
		1.76,1.64,1.38,1.51,1.71,1.26,1.50,1.23,1.19,1.46,0.99,1.00,0.91,1.70,1.85,1.65,1.93,1.54,1.76,1.52,1.48,1.26,1.28,1.39,1.09,0.99,0.97,1.18,1.31,1.01,1.05,0.90,
		0.93,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.86,1.95,1.82,1.98,1.89,1.71,1.80,1.91,1.77,1.86,1.67,1.34,1.53,1.51,1.72,1.92,1.91,1.99,1.69,1.82,1.80,1.94,1.87,1.86,1.97,1.59,1.44,1.69,1.05,1.24,1.27,1.49,
		1.50,1.69,1.72,1.63,1.46,1.37,1.00,1.23,0.98,0.95,1.09,0.96,1.16,1.55,0.99,1.25,1.10,1.24,1.41,0.90,0.85,0.94,0.79,0.81,0.85,0.89,0.94,1.09,0.98,0.89,0.82,0.83,
		0.74,0.77,0.72,0.76,0.73,0.75,0.78,0.94,0.86,0.91,0.79,0.83,0.89,0.99,0.95,0.90,0.90,0.92,0.84,0.86,0.79,0.75,0.81,0.85,0.80,0.78,0.76,0.77,0.73,0.74,0.71,0.71,
		0.73,0.74,0.74,0.71,0.76,0.72,0.70,0.79,0.85,0.78,0.98,0.92,0.93,0.85,0.87,0.82,0.79,0.81,0.89,0.86,1.16,0.97,1.12,0.95,1.06,1.00,0.93,1.38,1.60,1.35,1.77,1.71,
		1.57,1.48,1.20,1.28,1.62,1.27,1.46,1.17,1.05,1.34,0.96,0.99,0.90,1.63,1.74,1.50,1.80,1.33,1.58,1.48,1.37,1.21,1.04,1.21,0.97,0.97,0.93,1.05,1.34,1.02,1.14,0.84,
		0.88,0.92,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.74,1.89,1.76,1.98,1.89,1.71,1.93,1.99,1.91,1.94,1.82,1.46,1.60,1.65,1.80,1.79,1.77,1.92,1.57,1.69,1.74,1.87,1.88,1.94,1.98,1.53,1.45,1.70,1.18,1.32,1.42,1.58,
		1.65,1.83,1.81,1.81,1.67,1.61,1.19,1.44,1.17,1.11,1.36,1.15,1.41,1.75,1.22,1.50,1.34,1.42,1.61,0.98,0.92,1.03,0.83,0.86,0.89,0.95,0.98,1.23,1.14,0.97,0.89,0.90,
		0.78,0.82,0.76,0.82,0.77,0.79,0.84,0.98,0.90,0.98,0.83,0.89,0.97,1.03,0.95,0.92,0.86,0.90,0.82,0.86,0.79,0.77,0.84,0.81,0.76,0.76,0.72,0.73,0.70,0.72,0.71,0.73,
		0.73,0.72,0.74,0.71,0.78,0.74,0.72,0.75,0.80,0.76,0.94,0.88,0.91,0.83,0.87,0.84,0.79,0.76,0.82,0.80,0.97,0.89,0.96,0.88,0.95,0.94,0.87,1.11,1.37,1.10,1.59,1.57,
		1.37,1.33,1.05,1.08,1.54,1.34,1.46,1.16,0.99,1.26,0.96,1.05,0.92,1.45,1.55,1.27,1.60,1.07,1.34,1.35,1.18,1.07,0.93,0.99,0.90,0.93,0.87,0.96,1.27,0.99,1.15,0.77,
		0.82,0.85,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.60,1.78,1.68,1.93,1.86,1.71,1.97,1.99,1.99,1.97,1.93,1.60,1.68,1.78,1.86,1.61,1.57,1.79,1.37,1.48,1.59,1.72,1.79,1.92,1.90,1.38,1.35,1.60,1.23,1.30,1.47,1.56,
		1.71,1.88,1.79,1.92,1.79,1.79,1.30,1.56,1.35,1.37,1.59,1.38,1.60,1.90,1.48,1.72,1.57,1.61,1.79,1.21,1.00,1.30,0.89,0.94,0.96,1.07,1.14,1.40,1.37,1.14,0.96,0.98,
		0.82,0.88,0.82,0.89,0.83,0.86,0.91,1.02,0.93,1.07,0.87,0.94,1.11,1.02,0.93,0.93,0.82,0.87,0.80,0.85,0.79,0.80,0.85,0.77,0.72,0.74,0.71,0.70,0.70,0.71,0.72,0.77,
		0.74,0.72,0.76,0.73,0.82,0.79,0.76,0.73,0.79,0.76,0.93,0.86,0.91,0.83,0.89,0.89,0.82,0.72,0.76,0.76,0.89,0.82,0.89,0.82,0.89,0.91,0.83,0.96,1.14,0.97,1.40,1.44,
		1.19,1.22,0.99,0.98,1.49,1.44,1.49,1.22,0.99,1.23,0.98,1.19,0.97,1.21,1.30,1.00,1.37,0.94,1.07,1.14,0.98,0.96,0.86,0.91,0.83,0.88,0.82,0.89,1.11,0.94,1.07,0.73,
		0.76,0.79,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.46,1.65,1.60,1.82,1.80,1.71,1.93,1.91,1.99,1.94,1.98,1.74,1.76,1.89,1.89,1.42,1.34,1.61,1.11,1.22,1.36,1.50,1.61,1.81,1.75,1.15,1.17,1.41,1.18,1.19,1.42,1.44,
		1.65,1.83,1.67,1.94,1.81,1.88,1.32,1.58,1.45,1.57,1.74,1.53,1.70,1.98,1.69,1.87,1.77,1.79,1.92,1.45,1.27,1.55,0.97,1.07,1.11,1.34,1.37,1.59,1.60,1.35,1.07,1.18,
		0.86,0.93,0.87,0.96,0.90,0.93,0.99,1.03,0.95,1.15,0.90,0.99,1.27,0.98,0.90,0.92,0.78,0.83,0.77,0.84,0.79,0.82,0.86,0.73,0.71,0.73,0.72,0.70,0.73,0.72,0.76,0.81,
		0.76,0.76,0.82,0.77,0.89,0.85,0.82,0.75,0.80,0.80,0.94,0.88,0.94,0.87,0.95,0.96,0.88,0.72,0.74,0.76,0.83,0.78,0.84,0.79,0.87,0.91,0.83,0.89,0.98,0.92,1.23,1.34,
		1.05,1.16,0.99,0.96,1.46,1.57,1.54,1.33,1.05,1.26,1.08,1.37,1.10,0.98,1.03,0.92,1.14,0.86,0.95,0.97,0.90,0.89,0.79,0.84,0.77,0.82,0.76,0.82,0.97,0.89,0.98,0.71,
		0.72,0.74,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.34,1.51,1.53,1.67,1.72,1.71,1.80,1.77,1.91,1.86,1.98,1.86,1.82,1.95,1.89,1.24,1.10,1.41,0.95,0.99,1.09,1.25,1.37,1.63,1.55,0.96,0.98,1.16,1.05,1.00,1.27,1.23,
		1.50,1.69,1.46,1.86,1.72,1.87,1.24,1.49,1.44,1.69,1.80,1.59,1.69,1.97,1.82,1.94,1.91,1.92,1.99,1.63,1.50,1.74,1.16,1.33,1.38,1.58,1.60,1.77,1.80,1.48,1.21,1.37,
		0.90,0.97,0.93,1.05,0.97,1.04,1.21,0.99,0.95,1.14,0.92,1.02,1.34,0.94,0.86,0.90,0.74,0.79,0.75,0.81,0.79,0.84,0.86,0.71,0.71,0.73,0.76,0.73,0.77,0.74,0.80,0.85,
		0.78,0.81,0.89,0.84,0.97,0.92,0.88,0.79,0.85,0.86,0.98,0.92,1.00,0.93,1.06,1.12,0.95,0.74,0.74,0.78,0.79,0.76,0.82,0.79,0.87,0.93,0.85,0.85,0.94,0.90,1.09,1.27,
		0.99,1.17,1.05,0.96,1.46,1.71,1.62,1.48,1.20,1.34,1.28,1.57,1.35,0.90,0.94,0.85,0.98,0.81,0.89,0.89,0.83,0.82,0.75,0.78,0.73,0.77,0.72,0.76,0.89,0.83,0.91,0.71,
		0.70,0.72,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	},
	{
		1.26,1.39,1.48,1.51,1.64,1.71,1.60,1.58,1.77,1.74,1.91,1.94,1.87,1.97,1.85,1.10,0.97,1.22,0.88,0.92,0.95,1.01,1.11,1.39,1.32,0.88,0.90,0.97,0.96,0.93,1.05,0.99,
		1.27,1.47,1.20,1.70,1.54,1.76,1.08,1.31,1.33,1.70,1.76,1.55,1.57,1.88,1.85,1.91,1.97,1.99,1.99,1.70,1.65,1.85,1.41,1.54,1.61,1.76,1.80,1.91,1.93,1.52,1.26,1.48,
		0.92,0.99,0.97,1.18,1.09,1.28,1.39,0.94,0.93,1.05,0.92,1.01,1.31,0.88,0.81,0.86,0.72,0.75,0.74,0.79,0.79,0.86,0.85,0.71,0.73,0.75,0.82,0.77,0.83,0.78,0.85,0.88,
		0.81,0.88,0.97,0.90,1.18,1.00,0.93,0.86,0.92,0.94,1.14,0.99,1.24,1.03,1.33,1.39,1.11,0.79,0.77,0.84,0.79,0.77,0.84,0.83,0.90,0.98,0.91,0.85,0.92,0.91,1.02,1.26,
		1.00,1.23,1.19,0.99,1.50,1.84,1.71,1.64,1.38,1.46,1.51,1.76,1.59,0.84,0.88,0.80,0.94,0.79,0.86,0.82,0.77,0.76,0.74,0.74,0.71,0.73,0.70,0.72,0.82,0.77,0.85,0.74,
		0.70,0.73,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,
		1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00,1.00
	}
};

vec3_t shadevector;
float* shadedots = r_avertexnormal_dots[ 0 ];

static model_t* backupModels[ MAX_MOD_KNOWN ];
static int numBackupModels = 0;

model_t* R_AllocModel() {
	if ( tr.numModels == MAX_MOD_KNOWN ) {
		return NULL;
	}

	model_t* mod = new model_t;
	Com_Memset( mod, 0, sizeof ( model_t ) );
	mod->index = tr.numModels;
	tr.models[ tr.numModels ] = mod;
	tr.numModels++;

	return mod;
}

static void R_LoadCacheModels() {
	if ( !r_cacheModels->integer ) {
		return;
	}

	// don't load the cache list in between level loads, only on startup, or after a vid_restart
	if ( numBackupModels > 0 ) {
		return;
	}

	char* buf;
	int len = FS_ReadFile( "model.cache", ( void** )&buf );
	if ( len <= 0 ) {
		return;
	}

	const char* pString = buf;

	char* token;
	while ( ( token = String::ParseExt( &pString, true ) ) && token[ 0 ] ) {
		char name[ MAX_QPATH ];
		String::NCpyZ( name, token, sizeof ( name ) );
		R_RegisterModel( name );
	}

	FS_FreeFile( buf );
}

void R_ModelInit() {
	// leave a space for NULL model
	tr.numModels = 0;

	model_t* mod = R_AllocModel();
	mod->type = MOD_BAD;

	R_InitBsp29NoTextureMip();

	R_LoadCacheModels();
}

static void R_FreeModel( model_t* mod ) {
	if ( mod->type == MOD_SPRITE ) {
		Mod_FreeSpriteModel( mod );
	} else if ( mod->type == MOD_SPRITE2 )     {
		Mod_FreeSprite2Model( mod );
	} else if ( mod->type == MOD_MESH1 )     {
		Mod_FreeMdlModel( mod );
	} else if ( mod->type == MOD_MESH2 )     {
		Mod_FreeMd2Model( mod );
	} else if ( mod->type == MOD_MESH3 )     {
		R_FreeMd3( mod );
	} else if ( mod->type == MOD_MD4 )     {
		R_FreeMd4( mod );
	} else if ( mod->type == MOD_MDC )     {
		R_FreeMdc( mod );
	} else if ( mod->type == MOD_MDS )     {
		R_FreeMds( mod );
	} else if ( mod->type == MOD_MDM )     {
		R_FreeMdm( mod );
	} else if ( mod->type == MOD_MDX )     {
		R_FreeMdx( mod );
	} else if ( mod->type == MOD_BRUSH29 )     {
		Mod_FreeBsp29( mod );
	} else if ( mod->type == MOD_BRUSH38 )     {
		Mod_FreeBsp38( mod );
	} else if ( mod->type == MOD_BRUSH46 )     {
		R_FreeBsp46Model( mod );
	}
	delete mod;
}

void R_FreeModels() {
	for ( int i = 0; i < tr.numModels; i++ ) {
		R_FreeModel( tr.models[ i ] );
		tr.models[ i ] = NULL;
	}
	tr.numModels = 0;

	if ( tr.world ) {
		R_FreeBsp46( tr.world );
	}
	tr.worldMapLoaded = false;

	Mem_Free( r_notexture_mip );
	r_notexture_mip = NULL;
}

void R_LoadWorld( const char* name ) {
	if ( tr.worldMapLoaded ) {
		common->Error( "ERROR: attempted to redundantly load world map\n" );
	}

	skyboxportal = 0;

	// set default sun direction to be used if it isn't
	// overridden by a shader
	tr.sunDirection[ 0 ] = 0.45f;
	tr.sunDirection[ 1 ] = 0.3f;
	tr.sunDirection[ 2 ] = 0.9f;

	VectorNormalize( tr.sunDirection );

	tr.sunShader = 0;	// clear sunshader so it's not there if the level doesn't specify it

	// invalidate fogs (likely to be re-initialized to new values by the current map)
	// TODO:(SA)this is sort of silly.  I'm going to do a general cleanup on fog stuff
	//			now that I can see how it's been used.  (functionality can narrow since
	//			it's not used as much as it's designed for.)
	R_SetFog( FOG_SKY, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_HUD, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_MAP, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_CURRENT, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_TARGET, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_WATER, 0, 0, 0, 0, 0, 0 );
	R_SetFog( FOG_SERVER, 0, 0, 0, 0, 0, 0 );

	tr.worldMapLoaded = true;
	tr.worldDir = NULL;

	// load it
	void* buffer;
	FS_ReadFile( name, &buffer );
	if ( !buffer ) {
		common->Error( "R_LoadWorld: %s not found", name );
	}

	model_t* mod = NULL;
	if ( !( GGameType & GAME_Tech3 ) ) {
		mod = R_AllocModel();
		String::NCpyZ( mod->name, name, sizeof ( mod->name ) );
		loadmodel = mod;
	}

	// ydnar: set map meta dir
	tr.worldDir = __CopyString( name );
	String::StripExtension( tr.worldDir, tr.worldDir );

	// clear tr.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	tr.world = NULL;

	Com_Memset( &s_worldData, 0, sizeof ( s_worldData ) );
	String::NCpyZ( s_worldData.name, name, sizeof ( s_worldData.name ) );

	String::NCpyZ( s_worldData.baseName, String::SkipPath( s_worldData.name ), sizeof ( s_worldData.name ) );
	String::StripExtension( s_worldData.baseName, s_worldData.baseName );

	if ( GGameType & GAME_QuakeHexen ) {
		Mod_LoadBrush29Model( mod, buffer );

		// identify sky texture
		skytexturenum = -1;
		for ( int i = 0; i < mod->brush29_numtextures; i++ ) {
			if ( !mod->brush29_textures[ i ] ) {
				continue;
			}
			if ( !String::NCmp( mod->brush29_textures[ i ]->name, "sky", 3 ) ) {
				skytexturenum = i;
			}
		}
	} else if ( GGameType & GAME_Quake2 )     {
		switch ( LittleLong( *( unsigned* )buffer ) ) {
		case BSP38_HEADER:
			Mod_LoadBrush38Model( mod, buffer );
			break;

		default:
			common->Error( "Mod_NumForName: unknown fileid for %s", name );
			break;
		}
	} else   {
		R_LoadBrush46Model( buffer );
	}

	// only set tr.world now that we know the entire level has loaded properly
	tr.world = &s_worldData;
	tr.worldModel = mod;

	if ( GGameType & ( GAME_WolfMP | GAME_ET ) ) {
		// reset fog to world fog (if present)
		R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0 );
	}

	//	set the sun shader if there is one
	if ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) && tr.sunShaderName ) {
		tr.sunShader = R_FindShader( tr.sunShaderName, LIGHTMAP_NONE, true );
	}

	FS_FreeFile( buffer );
}

bool R_GetEntityToken( char* buffer, int size ) {
	const char* s = String::Parse3( const_cast<const char**>( &s_worldData.entityParsePoint ) );
	String::NCpyZ( buffer, s, size );
	if ( !s_worldData.entityParsePoint || !s[ 0 ] ) {
		s_worldData.entityParsePoint = s_worldData.entityString;
		return false;
	} else   {
		return true;
	}
}

model_t* R_GetModelByHandle( qhandle_t index ) {
	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels ) {
		return tr.models[ 0 ];
	}

	return tr.models[ index ];
}

//	loads a model's shadow script
static void R_LoadModelShadow( model_t* mod ) {
	// set default shadow
	mod->q3_shadowShader = 0;

	// build name
	char filename[ 1024 ];
	String::StripExtension2( mod->name, filename, sizeof ( filename ) );
	String::DefaultExtension( filename, 1024, ".shadow" );

	// load file
	char* buf;
	FS_ReadFile( filename, ( void** )&buf );
	if ( !buf ) {
		return;
	}

	char* shadowBits = strchr( buf, ' ' );
	if ( shadowBits != NULL ) {
		*shadowBits = '\0';
		shadowBits++;

		if ( String::Length( buf ) >= MAX_QPATH ) {
			common->Printf( "R_LoadModelShadow: Shader name exceeds MAX_QPATH\n" );
			mod->q3_shadowShader = 0;
		} else   {
			shader_t* sh = R_FindShader( buf, LIGHTMAP_NONE, true );

			if ( sh->defaultShader ) {
				mod->q3_shadowShader = 0;
			} else   {
				mod->q3_shadowShader = sh->index;
			}
		}
		sscanf( shadowBits, "%f %f %f %f %f %f",
			&mod->q3_shadowParms[ 0 ], &mod->q3_shadowParms[ 1 ], &mod->q3_shadowParms[ 2 ],
			&mod->q3_shadowParms[ 3 ], &mod->q3_shadowParms[ 4 ], &mod->q3_shadowParms[ 5 ] );
	}
	FS_FreeFile( buf );
}

//	look for the given model in the list of backupModels
static bool R_FindCachedModel( const char* name, model_t* newmod ) {
	// NOTE TTimo
	// would need an r_cache check here too?

	if ( !r_cacheModels->integer ) {
		return false;
	}

	if ( !numBackupModels ) {
		return false;
	}

	model_t** mod = backupModels;
	for ( int i = 0; i < numBackupModels; i++, mod++ ) {
		if ( *mod && !String::NCmp( ( *mod )->name, name, sizeof ( ( *mod )->name ) ) ) {
			// copy it to a new slot
			int index = newmod->index;
			memcpy( newmod, *mod, sizeof ( model_t ) );
			newmod->index = index;
			switch ( ( *mod )->type ) {
			case MOD_MESH3:
				for ( int j = MD3_MAX_LODS - 1; j >= 0; j-- ) {
					if ( j < ( *mod )->q3_numLods && ( *mod )->q3_md3[ j ] ) {
						if ( ( j == MD3_MAX_LODS - 1 ) || ( ( *mod )->q3_md3[ j ] != ( *mod )->q3_md3[ j + 1 ] ) ) {
							newmod->q3_md3[ j ] = ( *mod )->q3_md3[ j ];
							R_RegisterMd3Shaders( newmod, j );
						} else   {
							newmod->q3_md3[ j ] = ( *mod )->q3_md3[ j + 1 ];
						}
					}
				}
				break;
			case MOD_MDC:
				for ( int j = MD3_MAX_LODS - 1; j >= 0; j-- ) {
					if ( j < ( *mod )->q3_numLods && ( *mod )->q3_mdc[ j ] ) {
						if ( ( j == MD3_MAX_LODS - 1 ) || ( ( *mod )->q3_mdc[ j ] != ( *mod )->q3_mdc[ j + 1 ] ) ) {
							newmod->q3_mdc[ j ] = ( *mod )->q3_mdc[ j ];
							R_RegisterMdcShaders( newmod, j );
						} else   {
							newmod->q3_mdc[ j ] = ( *mod )->q3_mdc[ j + 1 ];
						}
					}
				}
				break;
			default:
				return false;	// not supported yet
			}

			delete *mod;
			*mod = NULL;
			return true;
		}
	}

	return false;
}

//	Loads in a model for the given name
//	Zero will be returned if the model fails to load. An entry will be
// retained for failed models as an optimization to prevent disk rescanning
// if they are asked for again.
int R_RegisterModel( const char* name ) {
	if ( !name || !name[ 0 ] ) {
		common->Printf( "R_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( String::Length( name ) >= MAX_QPATH ) {
		common->Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	// Ridah, caching
	if ( r_cacheGathering->integer ) {
		Cbuf_ExecuteText( EXEC_NOW, va( "cache_usedfile model %s\n", name ) );
	}

	//
	// search the currently loaded models
	//
	for ( int hModel = 1; hModel < tr.numModels; hModel++ ) {
		model_t* mod = tr.models[ hModel ];
		if ( !String::ICmp( mod->name, name ) ) {
			if ( mod->type == MOD_BAD ) {
				return 0;
			}
			return hModel;
		}
	}

	// allocate a new model_t
	model_t* mod = R_AllocModel();
	if ( mod == NULL ) {
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: R_AllocModel() failed for '%s'\n", name );
		return 0;
	}

	// only set the name after the model has been successfully loaded
	String::NCpyZ( mod->name, name, sizeof ( mod->name ) );

	// GR - by default models are not tessellated
	mod->q3_ATI_tess = false;
	// GR - check if can be tessellated...
	//		make sure to tessellate model heads
	if ( GGameType & GAME_WolfSP && strstr( name, "head" ) ) {
		mod->q3_ATI_tess = true;
	}

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// Ridah, look for it cached
	if ( R_FindCachedModel( name, mod ) ) {
		R_LoadModelShadow( mod );
		return mod->index;
	}

	R_LoadModelShadow( mod );

	void* buf;
	int modfilelen = FS_ReadFile( name, &buf );
	if ( !buf && ( GGameType & ( GAME_WolfSP | GAME_WolfMP | GAME_ET ) ) ) {
		char filename[ 1024 ];
		String::Cpy( filename, name );

		//	try MDC first
		filename[ String::Length( filename ) - 1 ] = 'c';
		FS_ReadFile( filename, &buf );

		if ( !buf ) {
			// try MD3 second
			filename[ String::Length( filename ) - 1 ] = '3';
			FS_ReadFile( filename, &buf );
		}
	}
	if ( !buf ) {
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name );
		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		mod->type = MOD_BAD;
		return 0;
	}

	loadmodel = mod;

	//	call the apropriate loader
	bool loaded;
	switch ( LittleLong( *( qint32* )buf ) ) {
	case IDPOLYHEADER:
		Mod_LoadMdlModel( mod, buf );
		loaded = true;
		break;

	case RAPOLYHEADER:
		Mod_LoadMdlModelNew( mod, buf );
		loaded = true;
		break;

	case IDSPRITE1HEADER:
		Mod_LoadSpriteModel( mod, buf );
		loaded = true;
		break;

	case BSP29_VERSION:
		Mod_LoadBrush29Model( mod, buf );
		loaded = true;
		break;

	case IDMESH2HEADER:
		Mod_LoadMd2Model( mod, buf );
		loaded = true;
		break;

	case IDSPRITE2HEADER:
		Mod_LoadSprite2Model( mod, buf, modfilelen );
		loaded = true;
		break;

	case MD3_IDENT:
		loaded = R_LoadMd3( mod, buf );
		break;

	case MD4_IDENT:
		loaded = R_LoadMD4( mod, buf, name );
		break;

	case MDC_IDENT:
		loaded = R_LoadMdc( mod, buf );
		break;

	case MDS_IDENT:
		loaded = R_LoadMds( mod, buf );
		break;

	case MDM_IDENT:
		loaded = R_LoadMdm( mod, buf );
		break;

	case MDX_IDENT:
		loaded = R_LoadMdx( mod, buf );
		break;

	default:
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: unknown fileid for %s\n", name );
		loaded = false;
	}

	FS_FreeFile( buf );

	if ( !loaded ) {
		common->Printf( S_COLOR_YELLOW "R_RegisterModel: couldn't load %s\n", name );
		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		mod->type = MOD_BAD;
		return 0;
	}

	return mod->index;
}

void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs ) {
	model_t* model = R_GetModelByHandle( handle );

	switch ( model->type ) {
	case MOD_BRUSH29:
	case MOD_SPRITE:
	case MOD_MESH1:
		VectorCopy( model->q1_mins, mins );
		VectorCopy( model->q1_maxs, maxs );
		break;

	case MOD_BRUSH38:
	case MOD_MESH2:
		VectorCopy( model->q2_mins, mins );
		VectorCopy( model->q2_maxs, maxs );
		break;

	case MOD_BRUSH46:
		VectorCopy( model->q3_bmodel->bounds[ 0 ], mins );
		VectorCopy( model->q3_bmodel->bounds[ 1 ], maxs );
		break;

	case MOD_MESH3:
	{
		md3Header_t* header = model->q3_md3[ 0 ];

		md3Frame_t* frame = ( md3Frame_t* )( ( byte* )header + header->ofsFrames );

		VectorCopy( frame->bounds[ 0 ], mins );
		VectorCopy( frame->bounds[ 1 ], maxs );
	}
	break;

	case MOD_MDC:
	{
		md3Frame_t* frame = ( md3Frame_t* )( ( byte* )model->q3_mdc[ 0 ] + model->q3_mdc[ 0 ]->ofsFrames );

		VectorCopy( frame->bounds[ 0 ], mins );
		VectorCopy( frame->bounds[ 1 ], maxs );
	}
	break;

	default:
		VectorClear( mins );
		VectorClear( maxs );
		break;
	}
}

int R_ModelNumFrames( qhandle_t Handle ) {
	model_t* Model = R_GetModelByHandle( Handle );
	switch ( Model->type ) {
	case MOD_BRUSH29:
	case MOD_SPRITE:
	case MOD_MESH1:
		return Model->q1_numframes;

	case MOD_BRUSH38:
	case MOD_SPRITE2:
	case MOD_MESH2:
		return Model->q2_numframes;

	case MOD_BRUSH46:
		return 1;

	case MOD_MESH3:
		return Model->q3_md3[ 0 ]->numFrames;

	case MOD_MD4:
		return Model->q3_md4->numFrames;

	case MOD_MDC:
		return Model->q3_mdc[ 0 ]->numFrames;

	case MOD_MDS:
		return Model->q3_mds->numFrames;

	case MOD_MDM:
		return 1;

	case MOD_MDX:
		return Model->q3_mdx->numFrames;

	default:
		return 0;
	}
}

//	Use only by Quake and Hexen 2, only .MDL files will have meaningfull flags.
int R_ModelFlags( qhandle_t Handle ) {
	model_t* Model = R_GetModelByHandle( Handle );
	if ( Model->type == MOD_MESH1 ) {
		return Model->q1_flags;
	}
	return 0;
}

bool R_IsMeshModel( qhandle_t Handle ) {
	model_t* Model = R_GetModelByHandle( Handle );
	return Model->type == MOD_MESH1 || Model->type == MOD_MESH2 ||
		   Model->type == MOD_MESH3 || Model->type == MOD_MD4 ||
		   Model->type == MOD_MDC || Model->type == MOD_MDS || Model->type == MOD_MDM;
}

const char* R_ModelName( qhandle_t Handle ) {
	return R_GetModelByHandle( Handle )->name;
}

int R_ModelSyncType( qhandle_t Handle ) {
	model_t* Model = R_GetModelByHandle( Handle );
	switch ( Model->type ) {
	case MOD_SPRITE:
	case MOD_MESH1:
		return Model->q1_synctype;

	default:
		return 0;
	}
}

void R_CalculateModelScaleOffset( qhandle_t Handle, float ScaleX, float ScaleY, float ScaleZ, float ScaleZOrigin, vec3_t Out ) {
	model_t* Model = R_GetModelByHandle( Handle );
	if ( Model->type != MOD_MESH1 ) {
		VectorClear( Out );
		return;
	}

	mesh1hdr_t* AliasHdr = ( mesh1hdr_t* )Model->q1_cache;

	Out[ 0 ] = -( ScaleX - 1.0 ) * ( AliasHdr->scale[ 0 ] * 127.95 + AliasHdr->scale_origin[ 0 ] );
	Out[ 1 ] = -( ScaleY - 1.0 ) * ( AliasHdr->scale[ 1 ] * 127.95 + AliasHdr->scale_origin[ 1 ] );
	Out[ 2 ] = -( ScaleZ - 1.0 ) * ( ScaleZOrigin * 2.0 * AliasHdr->scale[ 2 ] * 127.95 + AliasHdr->scale_origin[ 2 ] );
}

static md3Tag_t* R_GetTag( md3Header_t* mod, int frame, const char* tagName ) {
	md3Tag_t* tag;
	int i;

	if ( frame >= mod->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mod->numFrames - 1;
	}

	tag = ( md3Tag_t* )( ( byte* )mod + mod->ofsTags ) + frame * mod->numTags;
	for ( i = 0; i < mod->numTags; i++, tag++ ) {
		if ( !String::Cmp( tag->name, tagName ) ) {
			return tag;	// found it
		}
	}

	return NULL;
}

bool R_LerpTag( orientation_t* tag, qhandle_t handle, int startFrame, int endFrame,
	float frac, const char* tagName ) {
	int i;
	float frontLerp, backLerp;

	model_t* model = R_GetModelByHandle( handle );
	if ( !model->q3_md3[ 0 ] ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return false;
	}

	md3Tag_t* start = R_GetTag( model->q3_md3[ 0 ], startFrame, tagName );
	md3Tag_t* end = R_GetTag( model->q3_md3[ 0 ], endFrame, tagName );
	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return false;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0; i < 3; i++ ) {
		tag->origin[ i ] = start->origin[ i ] * backLerp +  end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp +  end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp +  end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp +  end->axis[ 2 ][ i ] * frontLerp;
	}
	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );
	return true;
}

static int R_GetTag( byte* mod, int frame, const char* tagName, int startTagIndex, md3Tag_t** outTag ) {
	md3Header_t* md3 = ( md3Header_t* )mod;

	if ( frame >= md3->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = md3->numFrames - 1;
	}

	if ( startTagIndex > md3->numTags ) {
		*outTag = NULL;
		return -1;
	}

	md3Tag_t* tag = ( md3Tag_t* )( ( byte* )mod + md3->ofsTags ) + frame * md3->numTags;
	for ( int i = 0; i < md3->numTags; i++, tag++ ) {
		if ( ( i >= startTagIndex ) && !String::Cmp( tag->name, tagName ) ) {
			*outTag = tag;
			return i;	// found it
		}
	}

	*outTag = NULL;
	return -1;
}

static int R_GetMDCTag( byte* mod, int frame, const char* tagName, int startTagIndex, mdcTag_t** outTag ) {
	mdcHeader_t* mdc = ( mdcHeader_t* )mod;

	if ( frame >= mdc->numFrames ) {
		// it is possible to have a bad frame while changing models, so don't error
		frame = mdc->numFrames - 1;
	}

	if ( startTagIndex > mdc->numTags ) {
		*outTag = NULL;
		return -1;
	}

	mdcTagName_t* pTagName = ( mdcTagName_t* )( ( byte* )mod + mdc->ofsTagNames );
	for ( int i = 0; i < mdc->numTags; i++, pTagName++ ) {
		if ( ( i >= startTagIndex ) && !String::Cmp( pTagName->name, tagName ) ) {
			mdcTag_t* tag = ( mdcTag_t* )( ( byte* )mod + mdc->ofsTags ) + frame * mdc->numTags + i;
			*outTag = tag;
			return i;
		}
	}

	*outTag = NULL;
	return -1;
}

//	returns the index of the tag it found, for cycling through tags with the same name
int R_LerpTag( orientation_t* tag, const refEntity_t* refent, const char* tagName, int startIndex ) {

	qhandle_t handle = refent->hModel;
	int startFrame = refent->oldframe;
	int endFrame = refent->frame;
	float frac = 1.0 - refent->backlerp;

	model_t* model = R_GetModelByHandle( handle );
	if ( !model->q3_md3[ 0 ] && !model->q3_mdc[ 0 ] && !model->q3_mds && !model->q3_mdm ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	float frontLerp = frac;
	float backLerp = 1.0 - frac;

	int retval;
	md3Tag_t* start;
	md3Tag_t* end;
	md3Tag_t ustart, uend;
	if ( model->type == MOD_MESH3 ) {
		// old MD3 style
		retval = R_GetTag( ( byte* )model->q3_md3[ 0 ], startFrame, tagName, startIndex, &start );
		retval = R_GetTag( ( byte* )model->q3_md3[ 0 ], endFrame, tagName, startIndex, &end );

	} else if ( model->type == MOD_MDS )     {
		// use bone lerping
		retval = R_GetBoneTagMds( tag, model->q3_mds, startIndex, refent, tagName );

		if ( retval >= 0 ) {
			return retval;
		}

		// failed
		return -1;
	} else if ( model->type == MOD_MDM )     {
		// use bone lerping
		retval = R_MDM_GetBoneTag( tag, model->q3_mdm, startIndex, refent, tagName );

		if ( retval >= 0 ) {
			return retval;
		}

		// failed
		return -1;
	} else   {
		// psuedo-compressed MDC tags
		mdcTag_t* cstart;
		mdcTag_t* cend;

		retval = R_GetMDCTag( ( byte* )model->q3_mdc[ 0 ], startFrame, tagName, startIndex, &cstart );
		retval = R_GetMDCTag( ( byte* )model->q3_mdc[ 0 ], endFrame, tagName, startIndex, &cend );

		// uncompress the MDC tags into MD3 style tags
		if ( cstart && cend ) {
			vec3_t sangles, eangles;
			for ( int i = 0; i < 3; i++ ) {
				ustart.origin[ i ] = ( float )cstart->xyz[ i ] * MD3_XYZ_SCALE;
				uend.origin[ i ] = ( float )cend->xyz[ i ] * MD3_XYZ_SCALE;
				sangles[ i ] = ( float )cstart->angles[ i ] * MDC_TAG_ANGLE_SCALE;
				eangles[ i ] = ( float )cend->angles[ i ] * MDC_TAG_ANGLE_SCALE;
			}

			AnglesToAxis( sangles, ustart.axis );
			AnglesToAxis( eangles, uend.axis );

			start = &ustart;
			end = &uend;
		} else   {
			start = NULL;
			end = NULL;
		}
	}

	if ( !start || !end ) {
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	for ( int i = 0; i < 3; i++ ) {
		tag->origin[ i ] = start->origin[ i ] * backLerp +  end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp +  end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp +  end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp +  end->axis[ 2 ][ i ] * frontLerp;
	}

	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );

	return retval;
}

void R_Modellist_f() {
	int total = 0;
	for ( int i = 0; i < tr.numModels; i++ ) {
		model_t* mod = tr.models[ i ];
		int lods = 1;
		int DataSize = 0;
		switch ( mod->type ) {
		case MOD_BRUSH38:
		case MOD_SPRITE2:
		case MOD_MESH2:
			DataSize = tr.models[ i ]->q2_extradatasize;
			break;

		case MOD_BRUSH46:
		case MOD_MD4:
		case MOD_MDS:
		case MOD_MDM:
		case MOD_MDX:
			DataSize = mod->q3_dataSize;
			break;

		case MOD_MESH3:
			DataSize = mod->q3_dataSize;
			for ( int j = 1; j < MD3_MAX_LODS; j++ ) {
				if ( mod->q3_md3[ j ] && mod->q3_md3[ j ] != mod->q3_md3[ j - 1 ] ) {
					lods++;
				}
			}
			break;

		case MOD_MDC:
			DataSize = mod->q3_dataSize;
			for ( int j = 1; j < MD3_MAX_LODS; j++ ) {
				if ( mod->q3_mdc[ j ] && mod->q3_mdc[ j ] != mod->q3_mdc[ j - 1 ] ) {
					lods++;
				}
			}
			break;

		default:
			break;
		}
		common->Printf( "%8i : (%i) %s\n", DataSize, lods, mod->name );
		total += DataSize;
	}
	common->Printf( "%8i : Total models\n", total );
}

void R_PurgeModels( int count ) {
	static int lastPurged = 0;

	if ( !numBackupModels ) {
		return;
	}

	for ( int i = lastPurged; i < numBackupModels; i++ ) {
		if ( backupModels[ i ] ) {
			R_FreeModel( backupModels[ i ] );
			backupModels[ i ] = NULL;
			count--;
			if ( count <= 0 ) {
				lastPurged = i + 1;
				return;
			}
		}
	}

	lastPurged = 0;
	numBackupModels = 0;
}

void R_BackupModels() {
	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheModels->integer ) {
		return;
	}

	if ( numBackupModels ) {
		R_PurgeModels( numBackupModels + 1 );	// get rid of them all
	}

	// copy each model in memory across to the backupModels
	model_t** modBack = backupModels;
	for ( int i = 0; i < tr.numModels; i++ ) {
		model_t* mod = tr.models[ i ];

		if ( mod->type == MOD_MESH3 || mod->type == MOD_MDC ) {
			*modBack = mod;
			modBack++;
			numBackupModels++;
		} else   {
			R_FreeModel( mod );
		}
	}
	tr.numModels = 0;
}
