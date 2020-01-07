#pragma once

float interp_linear_2pt( float coeff, float* zeroth );
float interp_hermite_4pt( float coeff, float* zeroth );
float interp_hermite_4pt_ref( float coeff, float** zeroth );
