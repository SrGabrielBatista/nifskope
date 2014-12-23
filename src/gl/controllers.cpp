/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "controllers.h"
#include "options.h"

#include "glmesh.h"
#include "glnode.h"
#include "glparticles.h"
#include "glproperty.h"
#include "glscene.h"


// `NiControllerManager` blocks

ControllerManager::ControllerManager( Node * node, const QModelIndex & index )
	: Controller( index ), target( node )
{
}

bool ControllerManager::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		if ( target ) {
			Scene * scene = target->scene;
			QVector<qint32> lSequences = nif->getLinkArray( index, "Controller Sequences" );
			for ( const auto l : lSequences ) {
				QModelIndex iSeq = nif->getBlock( l, "NiControllerSequence" );

				if ( iSeq.isValid() ) {
					QString name = nif->get<QString>( iSeq, "Name" );

					if ( !scene->animGroups.contains( name ) ) {
						scene->animGroups.append( name );

						QMap<QString, float> tags = scene->animTags[name];

						QModelIndex iKeys = nif->getBlock( nif->getLink( iSeq, "Text Keys" ), "NiTextKeyExtraData" );
						QModelIndex iTags = nif->getIndex( iKeys, "Text Keys" );

						for ( int r = 0; r < nif->rowCount( iTags ); r++ ) {
							tags.insert( nif->get<QString>( iTags.child( r, 0 ), "Value" ), nif->get<float>( iTags.child( r, 0 ), "Time" ) );
						}

						scene->animTags[name] = tags;
					}
				}
			}
		}

		return true;
	}

	return false;
}

void ControllerManager::setSequence( const QString & seqname )
{
	const NifModel * nif = static_cast<const NifModel *>(iBlock.model());

	if ( target && iBlock.isValid() && nif ) {
		MultiTargetTransformController * multiTargetTransformer = 0;
		for ( Controller * c : target->controllers ) {
			if ( c->typeId() == "NiMultiTargetTransformController" ) {
				multiTargetTransformer = static_cast<MultiTargetTransformController *>(c);
				break;
			}
		}

		QVector<qint32> lSequences = nif->getLinkArray( iBlock, "Controller Sequences" );
		for ( const auto l : lSequences ) {
			QModelIndex iSeq = nif->getBlock( l, "NiControllerSequence" );

			if ( iSeq.isValid() && nif->get<QString>( iSeq, "Name" ) == seqname ) {
				start = nif->get<float>( iSeq, "Start Time" );
				stop = nif->get<float>( iSeq, "Stop Time" );
				phase = nif->get<float>( iSeq, "Phase" );
				frequency = nif->get<float>( iSeq, "Frequency" );

				QModelIndex iCtrlBlcks = nif->getIndex( iSeq, "Controlled Blocks" );

				for ( int r = 0; r < nif->rowCount( iCtrlBlcks ); r++ ) {
					QModelIndex iCB = iCtrlBlcks.child( r, 0 );

					QModelIndex iInterpolator = nif->getBlock( nif->getLink( iCB, "Interpolator" ), "NiInterpolator" );

					QString nodename = nif->get<QString>( iCB, "Node Name" );

					if ( nodename.isEmpty() ) {
						QModelIndex idx = nif->getIndex( iCB, "Node Name Offset" );
						nodename = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
					}

					QString proptype = nif->get<QString>( iCB, "Property Type" );

					if ( proptype.isEmpty() ) {
						QModelIndex idx = nif->getIndex( iCB, "Property Type Offset" );
						proptype = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
					}

					QString ctrltype = nif->get<QString>( iCB, "Controller Type" );

					if ( ctrltype.isEmpty() ) {
						QModelIndex idx = nif->getIndex( iCB, "Controller Type Offset" );
						ctrltype = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
					}

					QString var1 = nif->get<QString>( iCB, "Variable 1" );

					if ( var1.isEmpty() ) {
						QModelIndex idx = nif->getIndex( iCB, "Variable 1 Offset" );
						var1 = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
					}

					QString var2 = nif->get<QString>( iCB, "Variable 2" );

					if ( var2.isEmpty() ) {
						QModelIndex idx = nif->getIndex( iCB, "Variable 2 Offset" );
						var2 = idx.sibling( idx.row(), NifModel::ValueCol ).data( NifSkopeDisplayRole ).toString();
					}

					Node * node = target->findChild( nodename );

					if ( !node )
						continue;

					if ( ctrltype == "NiTransformController" && multiTargetTransformer ) {
						if ( multiTargetTransformer->setInterpolator( node, iInterpolator ) ) {
							multiTargetTransformer->start = start;
							multiTargetTransformer->stop = stop;
							multiTargetTransformer->phase = phase;
							multiTargetTransformer->frequency = frequency;
							continue;
						}
					}

					Controller * ctrl = node->findController( proptype, ctrltype, var1, var2 );

					if ( ctrl ) {
						ctrl->start = start;
						ctrl->stop = stop;
						ctrl->phase = phase;
						ctrl->frequency = frequency;

						ctrl->setInterpolator( iInterpolator );
					}
				}
			}
		}
	}
}


// `NiKeyframeController` blocks

KeyframeController::KeyframeController( Node * node, const QModelIndex & index )
	: Controller( index ), target( node ), lTrans( 0 ), lRotate( 0 ), lScale( 0 )
{
}

void KeyframeController::update( float time )
{
	if ( !(active && target) )
		return;

	time = ctrlTime( time );

	interpolate( target->local.rotation, iRotations, time, lRotate );
	interpolate( target->local.translation, iTranslations, time, lTrans );
	interpolate( target->local.scale, iScales, time, lScale );
}

bool KeyframeController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		iTranslations = nif->getIndex( iData, "Translations" );
		iRotations = nif->getIndex( iData, "Rotations" );

		if ( !iRotations.isValid() )
			iRotations = iData;

		iScales = nif->getIndex( iData, "Scales" );
		return true;
	}

	return false;
}


// `NiTransformController` blocks

TransformController::TransformController( Node * node, const QModelIndex & index )
	: Controller( index ), target( node )
{
}

void TransformController::update( float time )
{
	if ( !(active && target) )
		return;

	time = ctrlTime( time );

	if ( interpolator ) {
		interpolator->updateTransform( target->local, time );
	}
}

void TransformController::setInterpolator( const QModelIndex & iBlock )
{
	const NifModel * nif = static_cast<const NifModel *>(iBlock.model());

	if ( nif && iBlock.isValid() ) {
		if ( interpolator ) {
			delete interpolator;
			interpolator = 0;
		}

		if ( nif->isNiBlock( iBlock, "NiBSplineCompTransformInterpolator" ) ) {
			iInterpolator = iBlock;
			interpolator = new BSplineTransformInterpolator( this );
		} else if ( nif->isNiBlock( iBlock, "NiTransformInterpolator" ) ) {
			iInterpolator = iBlock;
			interpolator = new TransformInterpolator( this );
		}

		if ( interpolator ) {
			interpolator->update( nif, iInterpolator );
		}
	}
}


// `NiMultiTargetTransformController` blocks

MultiTargetTransformController::MultiTargetTransformController( Node * node, const QModelIndex & index )
	: Controller( index ), target( node )
{
}

void MultiTargetTransformController::update( float time )
{
	if ( !(active && target) )
		return;

	time = ctrlTime( time );

	for ( const TransformTarget& tt : extraTargets ) {
		if ( tt.first && tt.second ) {
			tt.second->updateTransform( tt.first->local, time );
		}
	}
}

bool MultiTargetTransformController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		if ( target ) {
			Scene * scene = target->scene;
			extraTargets.clear();

			QVector<qint32> lTargets = nif->getLinkArray( index, "Extra Targets" );
			for ( const auto l : lTargets ) {
				Node * node = scene->getNode( nif, nif->getBlock( l ) );

				if ( node ) {
					extraTargets.append( TransformTarget( node, 0 ) );
				}
			}
		}

		return true;
	}

	for ( const TransformTarget& tt : extraTargets ) {
		// TODO: update the interpolators
	}

	return false;
}

bool MultiTargetTransformController::setInterpolator( Node * node, const QModelIndex & iInterpolator )
{
	const NifModel * nif = static_cast<const NifModel *>(iInterpolator.model());

	if ( !nif || !iInterpolator.isValid() )
		return false;

	QMutableListIterator<TransformTarget> it( extraTargets );

	while ( it.hasNext() ) {
		it.next();

		if ( it.value().first == node ) {
			if ( it.value().second ) {
				delete it.value().second;
				it.value().second = 0;
			}

			if ( nif->isNiBlock( iInterpolator, "NiBSplineCompTransformInterpolator" ) ) {
				it.value().second = new BSplineTransformInterpolator( this );
			} else if ( nif->isNiBlock( iInterpolator, "NiTransformInterpolator" ) ) {
				it.value().second = new TransformInterpolator( this );
			}

			if ( it.value().second ) {
				it.value().second->update( nif, iInterpolator );
			}

			return true;
		}
	}

	return false;
}


// `NiVisController` blocks

VisibilityController::VisibilityController( Node * node, const QModelIndex & index )
	: Controller( index ), target( node ), visLast( 0 )
{
}

void VisibilityController::update( float time )
{
	if ( !(active && target) )
		return;

	time = ctrlTime( time );

	bool isVisible;

	if ( interpolate( isVisible, iData, time, visLast ) ) {
		target->flags.node.hidden = !isVisible;
	}
}

bool VisibilityController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		// iData already points to the NiVisData
		// note that nif.xml needs to have "Keys" not "Vis Keys" for interpolate() to work
		//iKeys = nif->getIndex( iData, "Data" );
		return true;
	}

	return false;
}


// `NiGeomMorpherController` blocks

MorphController::MorphController( Mesh * mesh, const QModelIndex & index )
	: Controller( index ), target( mesh )
{
}

MorphController::~MorphController()
{
	qDeleteAll( morph );
}

void MorphController::update( float time )
{
	if ( !(target && iData.isValid() && active && morph.count() > 1) )
		return;

	time = ctrlTime( time );

	if ( target->verts.count() != morph[0]->verts.count() )
		return;

	target->verts = morph[0]->verts;

	float x;

	for ( int i = 1; i < morph.count(); i++ ) {
		MorphKey * key = morph[i];

		if ( interpolate( x, key->iFrames, time, key->index ) ) {
			if ( x < 0 )
				x = 0;
			if ( x > 1 )
				x = 1;

			if ( x != 0 && target->verts.count() == key->verts.count() ) {
				for ( int v = 0; v < target->verts.count(); v++ )
					target->verts[v] += key->verts[v] * x;
			}
		}
	}

	target->updateBounds = true;
}

bool MorphController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		qDeleteAll( morph );
		morph.clear();

		QModelIndex midx = nif->getIndex( iData, "Morphs" );

		for ( int r = 0; r < nif->rowCount( midx ); r++ ) {
			QModelIndex iInterpolators, iInterpolatorWeights;

			if ( nif->checkVersion( 0, 0x14000005 ) ) {
				iInterpolators = nif->getIndex( iBlock, "Interpolators" );
			} else if ( nif->checkVersion( 0x14010003, 0 ) ) {
				iInterpolatorWeights = nif->getIndex( iBlock, "Interpolator Weights" );
			}

			QModelIndex iKey = midx.child( r, 0 );

			MorphKey * key = new MorphKey;
			key->index = 0;

			// this is ugly...
			if ( iInterpolators.isValid() ) {
				key->iFrames = nif->getIndex( nif->getBlock( nif->getLink( nif->getBlock( nif->getLink( iInterpolators.child( r, 0 ) ), "NiFloatInterpolator" ), "Data" ), "NiFloatData" ), "Data" );
			} else if ( iInterpolatorWeights.isValid() ) {
				key->iFrames = nif->getIndex( nif->getBlock( nif->getLink( nif->getBlock( nif->getLink( iInterpolatorWeights.child( r, 0 ), "Interpolator" ), "NiFloatInterpolator" ), "Data" ), "NiFloatData" ), "Data" );
			} else {
				key->iFrames = iKey;
			}

			key->verts = nif->getArray<Vector3>( nif->getIndex( iKey, "Vectors" ) );

			morph.append( key );
		}

		return true;
	}

	return false;
}


// `NiUVController` blocks

UVController::UVController( Mesh * mesh, const QModelIndex & index )
	: Controller( index ), target( mesh )
{
}

UVController::~UVController()
{
}

void UVController::update( float time )
{
	const NifModel * nif = static_cast<const NifModel *>(iData.model());
	QModelIndex uvGroups = nif->getIndex( iData, "UV Groups" );

	// U trans, V trans, U scale, V scale
	// see NiUVData compound in nif.xml
	float val[4] = { 0.0, 0.0, 1.0, 1.0 };

	if ( uvGroups.isValid() ) {
		for ( int i = 0; i < 4 && i < nif->rowCount( uvGroups ); i++ ) {
			interpolate( val[i], uvGroups.child( i, 0 ), ctrlTime( time ), luv );
		}

		// adjust coords; verified in SceneImmerse
		for ( int i = 0; i < target->coords[0].size(); i++ ) {
			// operating on pointers makes this too complicated, so we don't
			Vector2 current = target->coords[0][i];
			// scaling/tiling applied before translation
			// Note that scaling is relative to center!
			current += Vector2( -0.5, -0.5 );
			current = Vector2( current[0] * val[2], current[1] * val[3] );
			current += Vector2( -val[0], val[1] );
			current += Vector2( 0.5, 0.5 );
			target->coords[0][i] = current;
		}
	}

	target->updateData = true;
}

bool UVController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		// do stuff here
		return true;
	}

	return false;
}


float random( float r )
{
	return r * rand() / RAND_MAX;
}

Vector3 random( Vector3 v )
{
	v[0] *= random( 1.0 );
	v[1] *= random( 1.0 );
	v[2] *= random( 1.0 );
	return v;
}


// `NiParticleSystemController` and other blocks

ParticleController::ParticleController( Particles * particles, const QModelIndex & index )
	: Controller( index ), target( particles )
{
}

bool ParticleController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( !target )
		return false;

	if ( Controller::update( nif, index ) || (index.isValid() && iExtras.contains( index )) ) {
		emitNode = target->scene->getNode( nif, nif->getBlock( nif->getLink( iBlock, "Emitter" ) ) );
		emitStart = nif->get<float>( iBlock, "Emit Start Time" );
		emitStop = nif->get<float>( iBlock, "Emit Stop Time" );
		emitRate = nif->get<float>( iBlock, "Emit Rate" );
		emitRadius = nif->get<Vector3>( iBlock, "Start Random" );
		emitAccu = 0;
		emitLast = emitStart;

		spd = nif->get<float>( iBlock, "Speed" );
		spdRnd = nif->get<float>( iBlock, "Speed Random" );

		ttl = nif->get<float>( iBlock, "Lifetime" );
		ttlRnd = nif->get<float>( iBlock, "Lifetime Random" );

		inc = nif->get<float>( iBlock, "Vertical Direction" );
		incRnd = nif->get<float>( iBlock, "Vertical Angle" );

		dec = nif->get<float>( iBlock, "Horizontal Direction" );
		decRnd = nif->get<float>( iBlock, "Horizontal Angle" );

		size = nif->get<float>( iBlock, "Size" );
		grow = 0.0;
		fade = 0.0;

		list.clear();

		QModelIndex iParticles = nif->getIndex( iBlock, "Particles" );

		if ( iParticles.isValid() ) {
			emitMax = nif->get<int>( iBlock, "Num Particles" );
			int active = nif->get<int>( iBlock, "Num Valid" );

			//iParticles = nif->getIndex( iParticles, "Particles" );
			//if ( iParticles.isValid() )
			//{
			for ( int p = 0; p < active && p < nif->rowCount( iParticles ); p++ ) {
				Particle particle;
				particle.velocity = nif->get<Vector3>( iParticles.child( p, 0 ), "Velocity" );
				particle.lifetime = nif->get<float>( iParticles.child( p, 0 ), "Lifetime" );
				particle.lifespan = nif->get<float>( iParticles.child( p, 0 ), "Lifespan" );
				particle.lasttime = nif->get<float>( iParticles.child( p, 0 ), "Timestamp" );
				particle.vertex = nif->get<int>( iParticles.child( p, 0 ), "Vertex ID" );
				// Display saved particle start on initial load
				list.append( particle );
			}

			//}
		}

		if ( (nif->get<int>( iBlock, "Emit Flags" ) & 1) == 0 ) {
			emitRate = emitMax / (ttl + ttlRnd / 2);
		}

		iExtras.clear();
		grav.clear();
		iColorKeys = QModelIndex();
		QModelIndex iExtra = nif->getBlock( nif->getLink( iBlock, "Particle Extra" ) );

		while ( iExtra.isValid() ) {
			iExtras.append( iExtra );

			QString name = nif->itemName( iExtra );

			if ( name == "NiParticleGrowFade" ) {
				grow = nif->get<float>( iExtra, "Grow" );
				fade = nif->get<float>( iExtra, "Fade" );
			} else if ( name == "NiParticleColorModifier" ) {
				iColorKeys = nif->getIndex( nif->getBlock( nif->getLink( iExtra, "Color Data" ), "NiColorData" ), "Data" );
			} else if ( name == "NiGravity" ) {
				Gravity g;
				g.force = nif->get<float>( iExtra, "Force" );
				g.type = nif->get<int>( iExtra, "Type" );
				g.position = nif->get<Vector3>( iExtra, "Position" );
				g.direction = nif->get<Vector3>( iExtra, "Direction" );
				grav.append( g );
			}

			iExtra = nif->getBlock( nif->getLink( iExtra, "Next Modifier" ) );
		}

		return true;
	}

	return false;
}

void ParticleController::update( float time )
{
	if ( !(target && active) )
		return;

	localtime = ctrlTime( time );

	int n = 0;

	while ( n < list.count() ) {
		Particle & p = list[n];

		float deltaTime = (localtime > p.lasttime ? localtime - p.lasttime : 0); //( stop - start ) - p.lasttime + localtime );

		p.lifetime += deltaTime;

		if ( p.lifetime < p.lifespan && p.vertex < target->verts.count() ) {
			p.position = target->verts[p.vertex];

			for ( int i = 0; i < 4; i++ )
				moveParticle( p, deltaTime / 4 );

			p.lasttime = localtime;
			n++;
		} else {
			list.remove( n );
		}
	}

	if ( emitNode && emitNode->isVisible() && localtime >= emitStart && localtime <= emitStop ) {
		float emitDelta = (localtime > emitLast ? localtime - emitLast : 0);
		emitLast = localtime;

		emitAccu += emitDelta * emitRate;

		int num = int( emitAccu );

		if ( num > 0 ) {
			emitAccu -= num;

			while ( num-- > 0 && list.count() < target->verts.count() ) {
				Particle p;
				startParticle( p );
				list.append( p );
			}
		}
	}

	n = 0;

	while ( n < list.count() ) {
		Particle & p = list[n];
		p.vertex = n;
		target->verts[n] = p.position;

		if ( n < target->sizes.count() )
			sizeParticle( p, target->sizes[n] );

		if ( n < target->colors.count() )
			colorParticle( p, target->colors[n] );

		n++;
	}

	target->active = list.count();
	target->size = size;
}

void ParticleController::startParticle( Particle & p )
{
	p.position = random( emitRadius * 2 ) - emitRadius;
	p.position += target->worldTrans().rotation.inverted() * (emitNode->worldTrans().translation - target->worldTrans().translation);

	float i = inc + random( incRnd );
	float d = dec + random( decRnd );

	p.velocity = Vector3( rand() & 1 ? sin( i ) : -sin( i ), 0, cos( i ) );

	Matrix m; m.fromEuler( 0, 0, rand() & 1 ? d : -d );
	p.velocity = m * p.velocity;

	p.velocity = p.velocity * (spd + random( spdRnd ));
	p.velocity = target->worldTrans().rotation.inverted() * emitNode->worldTrans().rotation * p.velocity;

	p.lifetime = 0;
	p.lifespan = ttl + random( ttlRnd );
	p.lasttime = localtime;
}

void ParticleController::moveParticle( Particle & p, float deltaTime )
{
	for ( Gravity g : grav ) {
		switch ( g.type ) {
		case 0:
			p.velocity += g.direction * (g.force * deltaTime);
			break;
		case 1:
		{
			Vector3 dir = (g.position - p.position);
			dir.normalize();
			p.velocity += dir * (g.force * deltaTime);
		}
		break;
		}
	}
	p.position += p.velocity * deltaTime;
}

void ParticleController::sizeParticle( Particle & p, float & size )
{
	size = 1.0;

	if ( grow > 0 && p.lifetime < grow )
		size *= p.lifetime / grow;

	if ( fade > 0 && p.lifespan - p.lifetime < fade )
		size *= (p.lifespan - p.lifetime) / fade;
}

void ParticleController::colorParticle( Particle & p, Color4 & color )
{
	if ( iColorKeys.isValid() ) {
		int i = 0;
		interpolate( color, iColorKeys, p.lifetime / p.lifespan, i );
	}
}


//! Controller for `NiFlipController`
TexFlipController::TexFlipController( TexturingProperty * prop, const QModelIndex & index )
	: Controller( index ), target( prop ), flipDelta( 0 ), flipSlot( 0 )
{
}

TexFlipController::TexFlipController( TextureProperty * prop, const QModelIndex & index )
	: Controller( index ), oldTarget( prop ), flipDelta( 0 ), flipSlot( 0 )
{
}

void TexFlipController::update( float time )
{
	const NifModel * nif = static_cast<const NifModel *>(iSources.model());

	if ( !((target || oldTarget) && active && iSources.isValid() && nif) )
		return;

	float r = 0;

	if ( iData.isValid() )
		interpolate( r, iData, "Data", ctrlTime( time ), flipLast );
	else if ( flipDelta > 0 )
		r = ctrlTime( time ) / flipDelta;

	// TexturingProperty
	if ( target ) {
		target->textures[flipSlot & 7].iSource = nif->getBlock( nif->getLink( iSources.child( (int)r, 0 ) ), "NiSourceTexture" );
	} else if ( oldTarget ) {
		oldTarget->iImage = nif->getBlock( nif->getLink( iSources.child( (int)r, 0 ) ), "NiImage" );
	}
}

bool TexFlipController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		flipDelta = nif->get<float>( iBlock, "Delta" );
		flipSlot = nif->get<int>( iBlock, "Texture Slot" );

		if ( nif->checkVersion( 0x04000000, 0 ) ) {
			iSources = nif->getIndex( iBlock, "Sources" );
		} else {
			iSources = nif->getIndex( iBlock, "Images" );
		}

		return true;
	}

	return false;
}


//! Controller for `NiTextureTransformController`
TexTransController::TexTransController( TexturingProperty * prop, const QModelIndex & index )
	: Controller( index ), target( prop ), texSlot( 0 ), texOP( 0 )
{
}

void TexTransController::update( float time )
{
	if ( !(target && active) )
		return;

	TexturingProperty::TexDesc * tex = &target->textures[texSlot & 7];

	float val;

	if ( interpolate( val, iData, "Data", ctrlTime( time ), lX ) ) {
		// If desired, we could force display even if texture transform was disabled:
		// tex->hasTransform = true;
		// however "Has Texture Transform" doesn't exist until 10.1.0.0, and neither does
		// NiTextureTransformController - so we won't bother
		switch ( texOP ) {
		case 0:
			tex->translation[0] = val;
			break;
		case 1:
			tex->translation[1] = val;
			break;
		case 2:
			tex->rotation = val;
			break;
		case 3:
			tex->tiling[0] = val;
			break;
		case 4:
			tex->tiling[1] = val;
			break;
		}
	}
}

bool TexTransController::update( const NifModel * nif, const QModelIndex & index )
{
	if ( Controller::update( nif, index ) ) {
		texSlot = nif->get<int>( iBlock, "Texture Slot" );
		texOP = nif->get<int>( iBlock, "Operation" );
		return true;
	}

	return false;
}
