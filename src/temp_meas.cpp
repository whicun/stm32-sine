/*
 * This file is part of the stm32-sine project.
 *
 * Copyright (C) 2010 Johannes Huebner <contact@johanneshuebner.com>
 * Copyright (C) 2010 Edward Cheeseman <cheesemanedward@gmail.com>
 * Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define __TEMP_LU_TABLES
#include "temp_meas.h"
#include <stdint.h>

#define TABLEN(a) sizeof(a) / sizeof(a[0])
#define FIRST_MOTOR_SENSOR TEMP_KTY83

enum coeff { PTC, NTC };

typedef struct TempSensor
{
   int16_t tempMin;
   int16_t tempMax;
   uint8_t step;
   uint8_t tabSize;
   enum coeff coeff;
   const uint16_t *lookup;
} TEMP_SENSOR;

/* Temp sensor with JCurve */
static const uint16_t JCurve[] = { JCURVE };

/* Temp sensor in Semikron Skiip82 module */
static const uint16_t Semikron[] = { SEMIKRON };

/* Temp sensor in MBB600 IGBT module */
static const uint16_t mbb600[] = { MBB600 };

/* Temp sensor in FS800 IGBT module */
static const uint16_t fs800[] = { FS800 };

/* Temp sensor KTY83-110 */
static const uint16_t Kty83[] = { KTY83 };

/* Temp sensor KTY84-130 */
static const uint16_t Kty84[] = { KTY84 };

/* Temp sensor in Nissan Leaf motor */
static const uint16_t leaf[] = { LEAF };

/* Temp sensor in Nissan Leaf Gen 2 inverter heat sink */
static const uint16_t leafhs[] = { LEAFHS };

static const uint16_t kty81m[] = { KTY81_M };

/* Temp sensor embedded in Tesla rear motor */
static const uint16_t Tesla100k[] = { TESLA_100K };

/* Temp sensor embedded in Tesla rear heatsink */
static const uint16_t Tesla52k[] = { TESLA_52K };

/* Coolant fluid sensor in Tesla LDU */
static const uint16_t TeslaFluid[] = { TESLA_LDU_FLUID };

/* Temp sensor embedded in Tesla rear heatsink */
static const uint16_t Tesla10k[] = { TESLA_10K };

/* Temp sensor embedded in many Toyota motors */
static const uint16_t Toyota[] = { TOYOTA_M };

/* Temp sensor embedded in many Toyota motors (Gen2 controller) */
static const uint16_t ToyotaGen2[] = { TOYOTA_MGEN2 };

/* contributed by Fabian Brauss */
/* Temp sensor KTY81-121 */
static const uint16_t Kty81hs[] = { KTY81_HS };

/* Temp sensor PT1000 */
static const uint16_t Pt1000[] = { PT1000 };

/* Temp sensor NTC K45 2k2 (with parallel 2k!) */
static const uint16_t NtcK45[] = { NTCK45 };

/* Temp sensor for outlander front motor (47k) in series with 1.2k with a 2.2k for R2 */
static const uint16_t OutlanderFront[] = { OUTLANDERFRONT };

/* EPCOS B57861-S 103-F40 temp sensor */
static const uint16_t epcosb57861[] = { EPCOSB57861 };

static const TEMP_SENSOR sensors[] =
{
   { -25, 105, 5,  TABLEN(JCurve),         NTC, JCurve     },
   { 0,   100, 5,  TABLEN(Semikron),       PTC, Semikron   },
   { -5,  100, 5,  TABLEN(mbb600),         PTC, mbb600     },
   { -50, 150, 10, TABLEN(Kty81hs),        NTC, Kty81hs    },
   { -50, 150, 10, TABLEN(Pt1000),         PTC, Pt1000     },
   { -50, 150, 5,  TABLEN(NtcK45),         NTC, NtcK45     },
   { -10, 160, 10, TABLEN(leafhs),         NTC, leafhs     },
   { -25, 105, 5,  TABLEN(fs800),          PTC, fs800      },
   { -50, 170, 10, TABLEN(Kty83),          PTC, Kty83      },
   { -40, 300, 10, TABLEN(Kty84),          PTC, Kty84      },
   { -20, 150, 10, TABLEN(leaf),           NTC, leaf       },
   { -50, 150, 10, TABLEN(kty81m),         PTC, kty81m     },
   { -20, 200, 5,  TABLEN(Toyota),         PTC, Toyota     },
   { -20, 200, 5,  TABLEN(Tesla100k),      PTC, Tesla100k  },
   { 0,   100, 10, TABLEN(Tesla52k),       PTC, Tesla52k   },
   { 5,   100,  5, TABLEN(TeslaFluid),     PTC, TeslaFluid },
   { -20, 190, 5,  TABLEN(Tesla10k),       PTC, Tesla10k   },
   { -40, 300, 10, TABLEN(OutlanderFront), NTC, OutlanderFront },
   { -50, 150, 10, TABLEN(epcosb57861),    NTC, epcosb57861},
   { -20, 200, 5,  TABLEN(ToyotaGen2),     NTC, ToyotaGen2 },
};

float TempMeas::Lookup(int digit, Sensors sensorId)
{
   if (sensorId >= TEMP_LAST) return 0;
   int index = sensorId >= FIRST_MOTOR_SENSOR ? sensorId - FIRST_MOTOR_SENSOR + NUM_HS_SENSORS : sensorId;

   const TEMP_SENSOR * sensor = &sensors[index];
   uint16_t last;

   for (uint32_t i = 0; i < sensor->tabSize; i++)
   {
      uint16_t cur = sensor->lookup[i];
      if ((sensor->coeff == NTC && cur >= digit) || (sensor->coeff == PTC && cur <= digit))
      {
         //we are outside the lookup table range, return minimum
         if (0 == i) return sensor->tempMin;
         float a = sensor->coeff == NTC?cur - digit:digit - cur;
         float b = sensor->coeff == NTC?cur - last:last - cur;
         float c = sensor->step * a / b;
         float d = (int)(sensor->step * i) + sensor->tempMin;
         return d - c;
      }
      last = cur;
   }
   return sensor->tempMax;
}
