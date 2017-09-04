/** @file opcontrol.c
 * @brief File for operator control code
 *
 * This file should contain the user operatorControl() function and any functions related to it.
 *
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * PROS contains FreeRTOS (http://www.freertos.org) whose source code may be
 * obtained from http://sourceforge.net/projects/freertos/files/ or on request.
 */

#include "main.h"
#include "TinyEKF.h"

class Fuser : public TinyEKF {
    public:
        Fuser() {
            // We approximate the process noise using a small constant
            this->setQ(0, 0, .0001);
            this->setQ(1, 1, .0001);

            // Same for measurement noise
            this->setR(0, 0, .0001);
            this->setR(1, 1, .0001);
            this->setR(2, 2, .0001);
        }
    protected:
        void model(float fx[Nsta], float F[Nsta][Nsta], float hx[Mobs], float H[Mobs][Nsta]) {
            // Process model is f(x) = x
            fx[0] = this->x[0];
            fx[1] = this->x[1];
						fx[2] = this->x[2];

            // So process model Jacobian is identity matrix
            F[0][0] = 1;
            F[1][1] = 1;
						F[2][2] = 1;

            // Measurement function
            hx[0] = this->x[0]; // Barometric pressure from previous state
            hx[1] = this->x[1]; // Baro temperature from previous state
            hx[2] = this->x[1]; // LM35 temperature from previous state

            // Jacobian of measurement function
            H[0][0] = 1;        // Barometric pressure from previous state
            H[1][1] = 1 ;       // Baro temperature from previous state
            H[2][1] = 1 ;       // LM35 temperature from previous state
        }
};

Fuser ekf;

void operatorControl() {
	float z[3] = {0};
	unsigned long start = millis();
	int iter =0 ;

	while (1) {
		z[0] = 0;
		z[1] = 0;
		z[2] = 0;
		ekf.step(z);
		iter++;
		if (iter == 20) {
			printf("time for 20 loops: %d\n", millis() - start);
			iter = 0;
			start = millis();
		}

		delay(15);
	}
}
