/*
 * filename :	Fresnel.h
 *
 * programmer :	Cao Jiayin
 */

#ifndef	SORT_FRESNEL
#define	SORT_FRESNEL

// include the headers
#include "spectrum/spectrum.h"

class	Fresnel
{
public:
	// evaluate spectrum
	virtual Spectrum Evaluate( float cosi , float coso) const = 0;
};

class	FresnelNo : public Fresnel
{
public:
	virtual Spectrum Evaluate( float cosi , float coso ) const
	{
		return 1.0f;
	}
};

class	FresnelConductor : public Fresnel
{
// public method
public:
	// constructor
	FresnelConductor( const Spectrum& e , const Spectrum& kk ):
		eta(e) , k(kk)
	{
	}

	/// evaluate spectrum
	virtual Spectrum Evaluate( float cosi , float coso ) const
	{
		float abs_cos = (cosi>0.0f)?cosi:(-cosi);
		float sq_cos = abs_cos * abs_cos;

		Spectrum t = 2 * eta * abs_cos;
		Spectrum tmp_f = eta*eta+k*k;
		Spectrum tmp = tmp_f * sq_cos;
		Spectrum Rparl2 = (tmp - t + 1 ) / ( tmp + t + 1 );
		Spectrum Rperp2 = (tmp_f - t + sq_cos)/(tmp_f + t + sq_cos );

		return (Rparl2+Rperp2)*0.5f;
	}

// private field
private:
	Spectrum eta , k ;
};

class	FresnelDielectric : public Fresnel
{
// public method
public:
	// constructor
	FresnelDielectric( float ei , float et ):
		eta_i(ei) , eta_t(et)
	{
	}

	// evalute spectrum
	virtual Spectrum Evaluate( float cosi , float coso ) const
	{
		float cos_i = ( cosi > 0.0f )?cosi:(-cosi);
		float cos_o = ( coso > 0.0f )?coso:(-coso);
		float t0 = eta_t * cos_i;
		float t1 = eta_i * cos_o;
		float t2 = eta_i * cos_i;
		float t3 = eta_t * cos_o;

		float Rparl = ( t0 - t1 ) / ( t0 + t1 );
		float Rparp = ( t2 - t3 ) / ( t2 + t3 );

		return ( Rparl * Rparl + Rparp * Rparp ) * 0.5f;
	}

// private field
private:
	float eta_t , eta_i;
};

#endif