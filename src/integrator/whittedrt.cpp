/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2018 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

// include the header file
#include "whittedrt.h"
#include "integratormethod.h"
#include "geometry/intersection.h"
#include "geometry/scene.h"
#include "bsdf/bsdf.h"
#include "light/light.h"
#include "log/log.h"

IMPLEMENT_CREATOR( WhittedRT );

// radiance along a specific ray direction
Spectrum WhittedRT::Li( const Ray& r , const PixelSample& ps ) const
{
	if( r.m_Depth > max_recursive_depth )
		return 0.0f;

	// get the intersection between the ray and the scene
	Intersection ip;
	if( false == scene.GetIntersect( r , &ip ) )
		return scene.Le(r);

	Spectrum t;

	// get bsdf
	Bsdf* bsdf = ip.primitive->GetMaterial()->GetBsdf( &ip );

	// lights
	Visibility visibility(scene);
	const vector<Light*>& lights = scene.GetLights();
	vector<Light*>::const_iterator it = lights.begin();
	while( it != lights.end() )
	{
		// only delta light is evaluated
		if( (*it)->IsDelta() )
		{
			Vector	lightDir;
			float	pdf;
			Spectrum ld = (*it)->sample_l( ip , &ps.light_sample[0] , lightDir , 0 , &pdf , 0 , 0 , visibility );
			if( ld.IsBlack() )
			{
				it++;
				continue;
			}
			Spectrum f = bsdf->f( -r.m_Dir , lightDir );
			if( f.IsBlack() )
			{
				it++;
				continue;
			}
			bool visible = visibility.IsVisible();
			if( visible )
				t += (ld * f * SatDot( lightDir , ip.normal ) / pdf);
		}
		it++;
	}

	return t;
}

// output log information
void WhittedRT::OutputLog() const{
    slog( INFO , INTEGRATOR , "Integrator algorithm : whitted ray tracing." );
}
