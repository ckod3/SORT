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

#include "bxdf_node.h"
#include "bsdf/lambert.h"
#include "bsdf/merl.h"
#include "bsdf/orennayar.h"
#include "bsdf/microfacet.h"
#include "managers/memmanager.h"
#include "bsdf/bsdf.h"

IMPLEMENT_CREATOR( LayeredBxdfNode );
IMPLEMENT_CREATOR( LambertNode );
IMPLEMENT_CREATOR( MerlNode );
IMPLEMENT_CREATOR( FourierBxdfNode );
IMPLEMENT_CREATOR( OrenNayarNode );
IMPLEMENT_CREATOR( MicrofacetReflectionNode );
IMPLEMENT_CREATOR( MicrofacetRefractionNode );

LayeredBxdfNode::LayeredBxdfNode(){
    for( int i = 0 ; i < MAX_BXDF_COUNT ; ++i ){
        m_props.insert( make_pair( "Bxdf" + to_string(i) , &bxdfs[i] ) );
        m_props.insert( make_pair( "Weight" + to_string(i) , &weights[i] ));
    }
}

bool LayeredBxdfNode::CheckValidation()
{
    for( auto bxdf_prop : bxdfs ){
        if( bxdf_prop.node ){
            // Only bxdf nodes are allowed to be connected to Layered Bxdf node
            MAT_NODE_TYPE sub_type = bxdf_prop.node->getNodeType();
            if( (sub_type & MAT_NODE_BXDF) == 0 )
                return false;
        }
    }
    
    for( auto weight_prop : weights ){
        if( weight_prop.node ){
            // Only constant node is allowed to be connected with Layered Bxdf weight node
            MAT_NODE_TYPE sub_type = weight_prop.node->getNodeType();
            if( (sub_type & MAT_NODE_CONSTANT) == 0 )
                return false;
        }
    }
    
    return MaterialNode::CheckValidation();
}

void LayeredBxdfNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
    for( int i = 0 ; i < MAX_BXDF_COUNT ; ++i )
        bxdfs[i].UpdateBsdf(bsdf, weights[i].GetPropertyValue(bsdf).ToSpectrum());
}

// check validation
bool BxdfNode::CheckValidation()
{
    for( auto prop : m_props ){
        MaterialNode* node = prop.second->node;
        if( node ){
            MAT_NODE_TYPE sub_type = node->getNodeType();
            
            // attaching bxdf result as an input of another bxdf doesn't make any sense at all
            if( sub_type & MAT_NODE_BXDF )
                return false;
        }
    }
    return MaterialNode::CheckValidation();
}

LambertNode::LambertNode()
{
	m_props.insert( make_pair( "BaseColor" , &baseColor ) );
}

void LambertNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
    Lambert* lambert = SORT_MALLOC(Lambert)( baseColor.GetPropertyValue(bsdf).ToSpectrum() );
	lambert->m_weight = weight;
	bsdf->AddBxdf( lambert );
}

MerlNode::MerlNode()
{
	m_props.insert( make_pair( "Filename" , &merlfile ) );
}

void MerlNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	merl.m_weight = weight;
	bsdf->AddBxdf( &merl );
}

// post process
void MerlNode::PostProcess()
{
	if( m_post_processed )
		return;

	if( merlfile.str.empty() == false )
		merl.LoadData( merlfile.str );
}

FourierBxdfNode::FourierBxdfNode()
{
    m_props.insert( make_pair( "Filename" , &fourierBxdfFile ) );
}

void FourierBxdfNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
    fourierBxdf.m_weight = weight;
    bsdf->AddBxdf( &fourierBxdf );
}

// post process
void FourierBxdfNode::PostProcess()
{
    if( m_post_processed )
        return;
    
    if( fourierBxdfFile.str.empty() == false )
        fourierBxdf.LoadData( fourierBxdfFile.str );
}

OrenNayarNode::OrenNayarNode()
{
	m_props.insert( make_pair( "BaseColor" , &baseColor ) );
	m_props.insert( make_pair( "Roughness" , &roughness ) );
}

void OrenNayarNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	OrenNayar* orennayar = SORT_MALLOC(OrenNayar)( baseColor.GetPropertyValue(bsdf).ToSpectrum() , roughness.GetPropertyValue(bsdf).x );
	orennayar->m_weight = weight;
	bsdf->AddBxdf( orennayar );
}

MicrofacetReflectionNode::MicrofacetReflectionNode()
{
	m_props.insert( make_pair( "BaseColor" , &baseColor ) );
	m_props.insert( make_pair( "MicroFacetDistribution" , &mf_dist ) );
	m_props.insert( make_pair( "Visibility" , &mf_vis ) );
	m_props.insert( make_pair( "Roughness" , &roughness ) );
	m_props.insert( make_pair( "eta" , &eta ) );
	m_props.insert( make_pair( "k" , &k ) );
}

void MicrofacetReflectionNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	float rn = clamp( roughness.GetPropertyValue(bsdf).x , 0.001f , 1.0f );
	MicroFacetDistribution* dist = 0;
	if( mf_dist.str == "Blinn" )
		dist = SORT_MALLOC(Blinn)( rn );
	else if( mf_dist.str == "Beckmann" )
		dist = SORT_MALLOC(Beckmann)( rn );
	else
		dist = SORT_MALLOC(GGX)( rn );	// GGX is default

	VisTerm* vis = 0;
	if( mf_vis.str == "Neumann" )
		vis = SORT_MALLOC(VisNeumann)();
	else if( mf_vis.str == "Kelemen" )
		vis = SORT_MALLOC(VisKelemen)();
	else if( mf_vis.str == "Schlick" )
		vis = SORT_MALLOC(VisSchlick)( rn );
	else if( mf_vis.str == "Smith" )
		vis = SORT_MALLOC(VisSmith)( rn );
	else if( mf_vis.str == "SmithJointApprox" )
		vis = SORT_MALLOC(VisSmithJointApprox)( rn );
	else if( mf_vis.str == "CookTorrance" )
		vis = SORT_MALLOC(VisCookTorrance)();
	else
		vis = new VisImplicit();	// implicit visibility term is default

	Fresnel* frenel = SORT_MALLOC( FresnelConductor )( eta.GetPropertyValue(bsdf).ToSpectrum() , k.GetPropertyValue(bsdf).ToSpectrum() );

	MicroFacetReflection* mf = SORT_MALLOC(MicroFacetReflection)( baseColor.GetPropertyValue(bsdf).ToSpectrum() , frenel , dist , vis);
	mf->m_weight = weight;
	bsdf->AddBxdf( mf );
}

MicrofacetRefractionNode::MicrofacetRefractionNode()
{
	m_props.insert( make_pair( "BaseColor" , &baseColor ) );
	m_props.insert( make_pair( "MicroFacetDistribution" , &mf_dist ) );
	m_props.insert( make_pair( "Visibility" , &mf_vis ) );
	m_props.insert( make_pair( "Roughness" , &roughness ) );
	m_props.insert( make_pair( "in_ior" , &in_ior ) );
	m_props.insert( make_pair( "ext_ior" , &ext_ior ) );
}

void MicrofacetRefractionNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	float rn = clamp( roughness.GetPropertyValue(bsdf).x , 0.05f , 1.0f );
	MicroFacetDistribution* dist = 0;
	if( mf_dist.str == "Blinn" )
		dist = SORT_MALLOC(Blinn)( rn );
	else if( mf_dist.str == "Beckmann" )
		dist = SORT_MALLOC(Beckmann)( rn );
	else
		dist = SORT_MALLOC(GGX)( rn );	// GGX is default

	VisTerm* vis = 0;
	if( mf_vis.str == "Neumann" )
		vis = SORT_MALLOC(VisNeumann)();
	else if( mf_vis.str == "Kelemen" )
		vis = SORT_MALLOC(VisKelemen)();
	else if( mf_vis.str == "Schlick" )
		vis = SORT_MALLOC(VisSchlick)( rn );
	else if( mf_vis.str == "Smith" )
		vis = SORT_MALLOC(VisSmith)( rn );
	else if( mf_vis.str == "SmithJointApprox" )
		vis = SORT_MALLOC(VisSmithJointApprox)( rn );
	else if( mf_vis.str == "CookTorrance" )
		vis = SORT_MALLOC(VisCookTorrance)();
	else
		vis = new VisImplicit();	// implicit visibility term is default

	float in_eta = in_ior.GetPropertyValue(bsdf).x;
	float ext_eta = ext_ior.GetPropertyValue(bsdf).x;
	Fresnel* frenel = SORT_MALLOC( FresnelDielectric )( in_eta , ext_eta );

	MicroFacetRefraction* mf = SORT_MALLOC(MicroFacetRefraction)( baseColor.GetPropertyValue(bsdf).ToSpectrum() , frenel , dist , vis , in_eta , ext_eta );
	mf->m_weight = weight;
	bsdf->AddBxdf( mf );
}
