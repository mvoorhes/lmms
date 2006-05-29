/*
 * oscillator.h - header-file for oscillator.cpp, a powerful oscillator-class
 *
 * Copyright (c) 2004-2006 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 * 
 * This file is part of Linux MultiMedia Studio - http://lmms.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */


#ifndef _OSCILLATOR_H
#define _OSCILLATOR_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "mixer.h"
#include "sample_buffer.h"
#include "lmms_constants.h"
#include "volume_knob.h"


// fwd-decl because we need it for the typedef below...
class oscillator;

typedef void ( oscillator:: * oscFuncPtr )
		( sampleFrame * _ab, const fpab_t _frames,
		  				const ch_cnt_t _chnl );


class oscillator
{
public:
	enum waveShapes
	{
		SIN_WAVE,
		TRIANGLE_WAVE,
		SAW_WAVE,
		SQUARE_WAVE,
		MOOG_SAW_WAVE,
		EXP_WAVE,
		WHITE_NOISE_WAVE,
		USER_DEF_WAVE
	} ;

	enum modulationAlgos
	{
		FREQ_MODULATION, AMP_MODULATION, MIX, SYNC
	} ;

	oscillator( const modulationAlgos _modulation_algo, const float _freq,
			const Sint16 _phase_offset, const float _volume_factor,
			const volumeKnob * _volume_knob,
			const sample_rate_t _sample_rate,
			oscillator * _m_subOsc ) FASTCALL;
	virtual ~oscillator()
	{
		delete m_subOsc;
	}

	inline void setUserWave( sampleBuffer * _wave )
	{
		m_userWave = _wave;
	}

	inline void update( sampleFrame * _ab, const fpab_t _frames,
							const ch_cnt_t _chnl )
	{
		( this->*m_callUpdate )( _ab, _frames, _chnl );
	}

	inline void setNewFreq( const float _new_freq )
	{
		// save current state - we need it later for restoring same
		// phase (otherwise we'll get clicks in the audio-stream)
		const float v = m_sample * m_oscCoeff;
		m_freq = _new_freq;
		recalcOscCoeff( fraction( v ) );
	}

	static oscillator * FASTCALL createOsc( const waveShapes _wave_shape,
					const modulationAlgos _modulation_algo,
					const float _freq,
					const Sint16 _phase_offset,
					const float _volume_factor,
					const volumeKnob * _volume_knob,
					const sample_rate_t _sample_rate,
						oscillator * _m_subOsc = NULL ); 
	inline bool syncOk( void )
	{
		const float v1 = m_sample * m_oscCoeff;
		const float v2 = ++m_sample * m_oscCoeff;
		// check whether v2 is in next period
		return( floorf( v2 ) > floorf( v1 ) );
	}
/*#define	FLOAT_TO_INT(in,out)		\
	register const float round_const = -0.5f;			\
	__asm__ __volatile__ ("fadd %%st,%%st(0)\n"		\
				"fadd	%2\n"			\
				"fistpl	%0\n"			\
				"shrl	$1,%0" : "=m" (out) : "t" (in),"m"(round_const) : "st") ;*/

	static inline float fraction( const float _sample )
	{
		return( _sample - static_cast<int>( _sample ) );
	}

	// now follow the wave-shape-routines...

	static inline sample_t sinSample( const float _sample )
	{
		return( sinf( _sample * F_2PI ) );
	}

	static inline sample_t triangleSample( const float _sample )
	{
		const float ph = fraction( _sample );
		if( ph <= 0.25f )
		{
			return( ph * 4.0f );
		}
		else if( ph <= 0.75f )
		{
			return( 2.0f - ph * 4.0f );
		}
		return( ph * 4.0f - 4.0f );
	}

	static inline sample_t sawSample( const float _sample )
	{
		return( -1.0f + fraction( _sample ) * 2.0f );
	}

	static inline sample_t squareSample( const float _sample )
	{
		return( ( fraction( _sample ) > 0.5f ) ? -1.0f : 1.0f );
	}

	static inline sample_t moogSawSample( const float _sample )
	{
		const float ph = fraction( _sample );
		if( ph < 0.5f )
		{
			return( -1.0f + ph * 4.0f );
		}
		return( 1.0f - 2.0f * ph );
	}

	static inline sample_t expSample( const float _sample )
	{
		float ph = fraction( _sample );
		if( ph > 0.5f )
		{
			ph = 1.0f - ph;
		}
		return( -1.0f + 8.0f * ph * ph );
	}

	static inline sample_t noiseSample( const float )
	{
		return( 1.0f - 2.0f * ( ( float )rand() * ( 1.0f /
								RAND_MAX ) ) );
	}

	inline sample_t userWaveSample( const float _sample )
	{
		if( m_userWave->frames() > 0 )
		{
			return( m_userWave->userWaveSample( _sample ) );
		}
		else
		{
			return( 0.0f );
		}
	}


protected:
	float m_freq;
	float m_volumeFactor;
	const volumeKnob * m_volumeKnob;
	Sint16 m_phaseOffset;
	const sample_rate_t m_sampleRate;
	oscillator * m_subOsc;
	f_cnt_t m_sample;
	float m_oscCoeff;
	sampleBuffer * m_userWave;
	oscFuncPtr m_callUpdate;


	float volumeFactor( void );

	virtual void FASTCALL updateNoSub( sampleFrame * _ab,
						const fpab_t _frames,
						const ch_cnt_t _chnl ) = 0;
	virtual void FASTCALL updateFM( sampleFrame * _ab,
						const fpab_t _frames,
						const ch_cnt_t _chnl ) = 0;
	virtual void FASTCALL updateAM( sampleFrame * _ab,
						const fpab_t _frames,
						const ch_cnt_t _chnl ) = 0;
	virtual void FASTCALL updateMix( sampleFrame * _ab,
						const fpab_t _frames,
						const ch_cnt_t _chnl ) = 0;
	virtual void FASTCALL updateSync( sampleFrame * _ab,
						const fpab_t _frames,
						const ch_cnt_t _chnl ) = 0;

	inline void sync( void )
	{
		m_sample = 0;
	}

	void FASTCALL recalcOscCoeff( const float _additional_phase_offset =
									0.0 );

} ;


#endif
