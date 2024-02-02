/*********************************************************************
Cosmological calculations.
This is part of GNU Astronomy Utilities (Gnuastro) package.

Original author:
     Mohammad Akhlaghi <mohammad@akhlaghi.org>
Contributing author(s):
Copyright (C) 2015-2024 Free Software Foundation, Inc.

Gnuastro is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

Gnuastro is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with Gnuastro. If not, see <http://www.gnu.org/licenses/>.
**********************************************************************/
#include <config.h>

#include <time.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_integration.h>




/**************************************************************/
/************             Definitions             *************/
/**************************************************************/
/* These are basic definitions that commonly go into the header files. But
   because this is a library and the user imports the header file, it is
   easier to just have them here in the main C file to avoid filling up the
   user's name-space with junk. */
struct cosmology_integrand_t
{
  double o_lambda_0;
  double o_curv_0;
  double o_matter_0;
  double o_radiation_0;
};





/* For the GSL integrations */
#define GSLILIMIT  1000
#define GSLIEPSABS 0
#define GSLIEPSREL 1e-7




















/**************************************************************/
/************     Constraint Check Function      *************/
/**************************************************************/
/* Check if input parameters are witihin the required constraints.
    i.e All density fractions should be between 0 and 1 AND their
    sum should not exceed 1. */
static void
cosmology_density_check(double o_lambda_0, double o_matter_0,
                        double o_radiation_0)
{
  double sum = o_lambda_0 + o_matter_0 + o_radiation_0;

  /* Check if the density fractions are between 0 and 1. */
  if(o_lambda_0 > 1 || o_lambda_0 < 0)
    error(EXIT_FAILURE, 0, "value to option 'olambda' must be between "
          "zero and one (inclusive), but the given value is '%.8f'. Recall "
          "that 'olambda' is 'Current cosmological cst. dens. per crit. '"
          "dens.", o_lambda_0);

  if(o_matter_0 > 1 || o_matter_0 < 0)
    error(EXIT_FAILURE, 0, "value to option 'omatter' must be between "
          "zero and one (inclusive), but the given value is '%.8f'. Recall "
          "that 'omatter' is 'Current matter density per critical density.'",
          o_matter_0);

  if(o_radiation_0 > 1 || o_radiation_0 < 0)
    error(EXIT_FAILURE, 0, "value to option 'oradiation' must be between "
          "zero and one (inclusive), but the given value is '%.8f'. Recall "
          "that 'oradiation' is 'Current radiation density per critical "
          "density.", o_radiation_0);

  /* Check if the density fractions add up to 1 (within floating point
      error). */
  if( sum > (1+1e-8) || sum < (1-1e-8) )
    error(EXIT_FAILURE, 0, "sum of fractional densities is not 1, "
          "but %.8f. The cosmological constant ('olambda'), matter "
          "('omatter') and radiation ('oradiation') densities are given "
          "as %.8f, %.8f, %.8f", sum, o_lambda_0, o_matter_0,
          o_radiation_0);

}




















/**************************************************************/
/************         Integrand functions         *************/
/**************************************************************/
/* These are integrands, they won't be giving the final value. */
static double
cosmology_integrand_Ez(double z, void *params)
{
  struct cosmology_integrand_t *p=(struct cosmology_integrand_t *)params;
  return sqrt( p->o_lambda_0
               + p->o_curv_0      * (1+z) * (1+z)
               + p->o_matter_0    * (1+z) * (1+z) * (1+z)
               + p->o_radiation_0 * (1+z) * (1+z) * (1+z) * (1+z));
}





static double
cosmology_integrand_age(double z, void *params)
{
  return 1 / ( (1.0 + z) * cosmology_integrand_Ez(z,params) );
}





static double
cosmology_integrand_proper_dist(double z, void *params)
{
  return 1 / ( cosmology_integrand_Ez(z,params) );
}





static double
cosmology_integrand_comoving_volume(double z, void *params)
{
  size_t neval;
  gsl_function F;
  double result, error;

  /* Set the GSL function parameters */
  F.params=params;
  F.function=&cosmology_integrand_proper_dist;

  gsl_integration_qng(&F, 0.0, z, GSLIEPSABS, GSLIEPSREL,
                      &result, &error, &neval);

  return result * result / ( cosmology_integrand_Ez(z,params) );
}




















/**************************************************************/
/************      Basic cosmology functions      *************/
/**************************************************************/
/* Age of the universe (in Gyrs). H0 is in units of (km/sec/Mpc) and the
   fractional densities must add up to 1. */
double
gal_cosmology_age(double z, double H0, double o_lambda_0, double o_matter_0,
                  double o_radiation_0)
{
  gsl_function F;
  double result, error;
  double o_curv_0 = 1.0 - ( o_lambda_0 + o_matter_0 + o_radiation_0 );
  double H0s=H0/1000/GSL_CONST_MKSA_PARSEC;  /* H0 in units of seconds. */
  gsl_integration_workspace *w=gsl_integration_workspace_alloc(GSLILIMIT);
  struct cosmology_integrand_t p={o_lambda_0, o_curv_0, o_matter_0,
                                  o_radiation_0};

  /* Basic sanity check (no problem with the usage of these variables in
     the definitions above; they are just  */
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);

  /* Set the GSL function parameters. */
  F.params=&p;
  F.function=&cosmology_integrand_age;
  gsl_integration_qagiu(&F, z, GSLIEPSABS, GSLIEPSREL, GSLILIMIT, w,
                        &result, &error);

  return result / H0s / (365*GSL_CONST_MKSA_DAY) / 1e9;
}





/* Proper distance to z (Mpc). */
double
gal_cosmology_proper_distance(double z, double H0, double o_lambda_0,
                              double o_matter_0, double o_radiation_0)
{
  int status;
  size_t neval;
  gsl_function F;
  gsl_integration_workspace *w;
  gsl_error_handler_t *gslerrhandler;
  double result, gslerr, c=GSL_CONST_MKSA_SPEED_OF_LIGHT;
  double o_curv_0 = 1.0 - ( o_lambda_0 + o_matter_0 + o_radiation_0 );
  double H0s=H0/1000/GSL_CONST_MKSA_PARSEC;  /* H0 in units of seconds. */
  struct cosmology_integrand_t p={o_lambda_0, o_curv_0, o_matter_0,
                                  o_radiation_0};

  /* Basic sanity check (no problem with the usage of these variables in
     the definitions above; they are just  */
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);

  /* Set the GSL function parameters */
  F.params=&p;
  F.function=&cosmology_integrand_proper_dist;

  /* Temporarily switch off error handling */
  gslerrhandler=gsl_set_error_handler_off();

  /* Do the integration (first with the fast GSL function). */
  status=gsl_integration_qng(&F, 0.0f, z, GSLIEPSABS, GSLIEPSREL, &result,
                             &gslerr, &neval);

  /* If the first integration failed, try a slower, but more robust one. */
  if (status!=GSL_SUCCESS)
    {
      w=gsl_integration_workspace_alloc(GSLILIMIT);
      status=gsl_integration_qag(&F, 0.0f, z, GSLIEPSABS, GSLIEPSREL,
                                 GSLILIMIT, GSL_INTEG_GAUSS21, w,
                                 &result, &gslerr);
      gsl_integration_workspace_free(w);
      if (status!=GSL_SUCCESS)
        error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
              "fix the problem. The status of the second integration is "
              "%i.",  __func__, PACKAGE_BUGREPORT, status);
  }

  /* Reactivate "normal" error handling */
  gslerrhandler=gsl_set_error_handler(gslerrhandler);

  /* Return the result. */
  return result * c / H0s / (1e6 * GSL_CONST_MKSA_PARSEC);
}





/* Comoving volume over 4pi stradian to z (Mpc^3). */
double
gal_cosmology_comoving_volume(double z, double H0, double o_lambda_0,
                              double o_matter_0, double o_radiation_0)
{
  int status;
  size_t neval;
  gsl_function F;
  double result, gslerr;
  gsl_integration_workspace *w;
  gsl_error_handler_t *gslerrhandler;
  double c=GSL_CONST_MKSA_SPEED_OF_LIGHT;
  double H0s=H0/1000/GSL_CONST_MKSA_PARSEC;     /* H0 in units of seconds. */
  double cH = c / H0s / (1e6 * GSL_CONST_MKSA_PARSEC);
  double o_curv_0 = 1.0 - ( o_lambda_0 + o_matter_0 + o_radiation_0 );
  struct cosmology_integrand_t p={o_lambda_0, o_curv_0, o_matter_0,
                                  o_radiation_0};

  /* Basic sanity check (no problem with the usage of these variables in
     the definitions above; they are just  */
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);

  /* Set the GSL function parameters */
  F.params=&p;
  F.function=&cosmology_integrand_comoving_volume;

  /* temporarily switch off error handling */
  gslerrhandler=gsl_set_error_handler_off();

  /* Do the integration. */
  status=gsl_integration_qng(&F, 0.0f, z, GSLIEPSABS, GSLIEPSREL,
                      &result, &gslerr, &neval);

  /* If the first integration failed, try a slower, but more robust one. */
  if (status!=GSL_SUCCESS)
    {
      w=gsl_integration_workspace_alloc(GSLILIMIT);
     status=gsl_integration_qag(&F, 0.0f, z, GSLIEPSABS, GSLIEPSREL,
                                GSLILIMIT, GSL_INTEG_GAUSS21, w,
                                &result, &gslerr);
     gsl_integration_workspace_free(w);
     if (status!=GSL_SUCCESS)
       error(EXIT_FAILURE, 0, "%s: a bug! Please contact us at '%s' to "
             "fix the problem. The status of the second integration is "
             "%i.",  __func__, PACKAGE_BUGREPORT, status);
    }

  /* Reactivate "normal" error handling */
  gslerrhandler=gsl_set_error_handler(gslerrhandler);

  /* Return the result. */
  return result * 4 * M_PI * cH*cH*cH;
}





/* Critical density at redshift z in units of g/cm^3. */
double
gal_cosmology_critical_density(double z, double H0, double o_lambda_0,
                               double o_matter_0, double o_radiation_0)
{
  double H;
  double H0s=H0/1000/GSL_CONST_MKSA_PARSEC;     /* H0 in units of seconds. */
  double o_curv_0 = 1.0 - ( o_lambda_0 + o_matter_0 + o_radiation_0 );
  struct cosmology_integrand_t p={o_lambda_0, o_curv_0, o_matter_0,
                                  o_radiation_0};

  /* Basic sanity check (no problem with the usage of these variables in
     the definitions above; they are just  */
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);

  /* Set the place holder, then return the result. */
  H = H0s * cosmology_integrand_Ez(z, &p);
  return 3*H*H/(8*M_PI*GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT)/1000;
}





/* Angular diameter distance to z (Mpc). */
double
gal_cosmology_angular_distance(double z, double H0, double o_lambda_0,
                               double o_matter_0, double o_radiation_0)
{
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);
  return gal_cosmology_proper_distance(z, H0, o_lambda_0, o_matter_0,
                                       o_radiation_0) / (1+z);
}





/* Luminosity distance to z (Mpc). */
double
gal_cosmology_luminosity_distance(double z, double H0, double o_lambda_0,
                                  double o_matter_0, double o_radiation_0)
{
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);
  return gal_cosmology_proper_distance(z, H0, o_lambda_0, o_matter_0,
                                       o_radiation_0) * (1+z);
}





/* Distance modulus at z (no units). */
double
gal_cosmology_distance_modulus(double z, double H0, double o_lambda_0,
                               double o_matter_0, double o_radiation_0)
{
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);
  double ld=gal_cosmology_luminosity_distance(z, H0, o_lambda_0, o_matter_0,
                                              o_radiation_0);
  return 5*(log10(ld*1000000)-1);
}





/* Convert apparent to absolute magnitude. */
double
gal_cosmology_to_absolute_mag(double z, double H0, double o_lambda_0,
                              double o_matter_0, double o_radiation_0)
{
  cosmology_density_check(o_lambda_0, o_matter_0, o_radiation_0);
  double dm=gal_cosmology_distance_modulus(z, H0, o_lambda_0, o_matter_0,
                                           o_radiation_0);
  return dm-2.5*log10(1.0+z);
}





/* Velocity at given redshift in units of km/s. */
double
gal_cosmology_velocity_from_z(double z)
{
  double c=GSL_CONST_MKSA_SPEED_OF_LIGHT;
  return c * ( (1+z)*(1+z) - 1 ) / ( (1+z)*(1+z) + 1 ) / 1000;
}





/* Redshift at given velocity (in units of km/s). */
double
gal_cosmology_z_from_velocity(double v)
{
  double c=GSL_CONST_MKSA_SPEED_OF_LIGHT/1000;
  return sqrt( (c+v)/(c-v) ) - 1;
}
