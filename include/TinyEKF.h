/*
 * TinyEKF: Extended Kalman Filter for Arduino and TeensyBoard.
 *
 * Copyright (C) 2015 Simon D. Levy
 *
 * This code is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this code.  If not, see <http:#www.gnu.org/licenses/>.
 */

#include <stdlib.h>

#define Nsta 3
#define Mobs 3

#include "tiny_ekf_struct.h"

void ekf_init(void *, int, int);
bool ekf_step(void *, float *);

/**
 * A header-only class for the Extended Kalman Filter.  Your implementing class should #define the constant N and
 * and then #include <TinyEKF.h>  You will also need to implement a model() method for your application.
 */
class TinyEKF {
    private:
        ekf_t ekf;

    protected:
        /**
          * The current state.
          */
        float *x;

        /**
         * Initializes a TinyEKF object.
         */
        TinyEKF() {
            ekf_init(&this->ekf, Nsta, Mobs);
            this->x = this->ekf.x;
        }

        /**
         * Deallocates memory for a TinyEKF object.
         */
        ~TinyEKF() {}

        /**
         * Implement this function for your EKF model.
         * @param fx gets output of state-transition function <i>f(x<sub>0 .. n-1</sub>)</i>
         * @param F gets <i>n &times; n</i> Jacobian of <i>f(x)</i>
         * @param hx gets output of observation function <i>h(x<sub>0 .. n-1</sub>)</i>
         * @param H gets <i>m &times; n</i> Jacobian of <i>h(x)</i>
         */
        virtual void model(float fx[Nsta], float F[Nsta][Nsta], float hx[Mobs], float H[Mobs][Nsta]) = 0;

        /**
         * Sets the specified value of the prediction error covariance. <i>P<sub>i,j</sub> = value</i>
         * @param i row index
         * @param j column index
         * @param value value to set
         */
        __attribute__((always_inline))
        void setP(const int i, const int j, const float value) {
            this->ekf.P[i][j] = value;
        }

        /**
         * Sets the specified value of the process noise covariance. <i>Q<sub>i,j</sub> = value</i>
         * @param i row index
         * @param j column index
         * @param value value to set
         */
        __attribute__((always_inline))
        void setQ(const int i, const int j, const float value) {
            this->ekf.Q[i][j] = value;
        }

        /**
         * Sets the specified value of the observation noise covariance. <i>R<sub>i,j</sub> = value</i>
         * @param i row index
         * @param j column index
         * @param value value to set
         */
        __attribute__((always_inline))
        void setR(const int i, const int j, const float value) {
            this->ekf.R[i][j] = value;
        }
    public:
        /**
         * Returns the state element at a given index.
         * @param i the index (at least 0 and less than <i>n</i>
         * @return state value at index
         */
        __attribute__((always_inline))
        float getX(const int i) {
            return this->ekf.x[i];
        }

        /**
         * Sets the state element at a given index.
         * @param i the index (at least 0 and less than <i>n</i>
         * @param value value to set
         */
        __attribute__((always_inline))
        void setX(const int i, const float value) {
            this->ekf.x[i] = value;
        }

        /**
          Performs one step of the prediction and update.
         * @param z observation vector, length <i>m</i>
         * @return true on success, false on failure caused by non-positive-definite matrix.
         */
        bool step(float *z) {
            this->model(this->ekf.fx, this->ekf.F, this->ekf.hx, this->ekf.H);
            return ekf_step(&this->ekf, z);
        }
};
