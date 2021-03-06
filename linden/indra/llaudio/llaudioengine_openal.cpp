/**
 * @file audioengine_openal.cpp
 * @brief implementation of audio engine using OpenAL
 * support as a OpenAL 3D implementation
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "linden_common.h"
#include "lldir.h"

#include "llaudioengine_openal.h"
#include "lllistener_openal.h"


static const F32 WIND_BUFFER_SIZE_SEC = 0.05f; // 1/20th sec

std::string convertALErrorToString(ALenum error)
{
    switch(error)
    {
    case AL_NO_ERROR:
		return std::string("AL_NO_ERROR");
		break;
    case AL_INVALID_NAME:
        return std::string("AL_INVALID_NAME");
		break;
    case AL_INVALID_ENUM:
        return std::string("AL_INVALID_ENUM");
		break;
    case AL_INVALID_VALUE:
        return std::string("AL_INVALID_VALUE");
		break;
    case AL_INVALID_OPERATION:
        return std::string("AL_INVALID_OPERATION");
		break;
    case AL_OUT_OF_MEMORY:
        return std::string("AL_OUT_OF_MEMORY");
		break;
	default:
		std::stringstream s;
		s << error;
		return s.str();
    }
}

LLAudioEngine_OpenAL::LLAudioEngine_OpenAL()
	:
	mWindGen(NULL),
	mWindBuf(NULL),
	mWindBufFreq(0),
	mWindBufSamples(0),
	mWindBufBytes(0),
	mWindSource(AL_NONE),
	mNumEmptyWindALBuffers(MAX_NUM_WIND_BUFFERS),
	mContext(NULL),
	mDevice(NULL)
{
}

// virtual
LLAudioEngine_OpenAL::~LLAudioEngine_OpenAL()
{
}

// virtual
bool LLAudioEngine_OpenAL::init(const S32 num_channels, void* userdata)
{
	mWindGen = NULL;
	LLAudioEngine::init(num_channels, userdata);

	if (!alutInit(NULL, NULL))
	{
		llwarns << "LLAudioEngine_OpenAL::init() ALUT initialization failed: " << alutGetErrorString (alutGetError ()) << llendl;
		return false;
	}

	// check for extensions
	const ALCchar* device_list(NULL);
	const ALCchar* device_default(NULL);
	if (alcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT") == AL_TRUE)
	{
		device_default = alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
		device_list = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
		llinfos << "Results for ALC_ENUMERATION_EXT:\n" 
				<< ll_safe_string(device_list)
				<< llendl;

	}

	// initialize device
    ALCdevice* mDevice = alcOpenDevice(NULL); 
    if (mDevice == NULL)
	{
		llinfos << "Could not find a default device, trying to open default manually: " 
				<< ll_safe_string(device_default) 
				<< llendl;
		mDevice = alcOpenDevice(device_default);
		if (mDevice == NULL)
		{
			const ALCchar* device_list_walk = device_list;
			do
			{
				mDevice = alcOpenDevice(device_list_walk);
				if (mDevice != NULL)
				{
					break;
				}
				else
				{
					device_list_walk += strlen(device_list_walk)+1;
				}
			}
			while (device_list_walk[0] != '\0');

			if (mDevice == NULL)
			{
				llinfos << "OpenAL could not find an installed audio device. Aborting" << llendl;
				ALCenum error = alcGetError(mDevice);
				if (error != ALC_NO_ERROR)
				{
					llinfos << "ALC error: " << ll_safe_string(alcGetString(mDevice, error)) << llendl;
				}
				return false;
			}
		}
	}

	// create context
	ALCcontext* mContext = alcCreateContext(mDevice, NULL);
	if (mContext != NULL)
	{
		if (!alcMakeContextCurrent(mContext))
		{
			ALenum error = alGetError();
			if (error != AL_NO_ERROR)
			{
				llinfos << "ALC error: " << convertALErrorToString(error) << ". Could not set current context!" << llendl;
			}
			alcDestroyContext(mContext);
			return false;
		}
	}
	else
	{
		llinfos << "ALC error: could not create context from device!" << llendl;
		alcCloseDevice(mDevice);
		return false;
	}

	llinfos << "LLAudioEngine_OpenAL::init() OpenAL successfully initialized" << llendl;

	llinfos << "ALC default device: " 
			<< ll_safe_string(alcGetString(mDevice, ALC_DEFAULT_DEVICE_SPECIFIER)) 
			<< llendl;

	llinfos << "OpenAL version: "
		<< ll_safe_string(alGetString(AL_VERSION)) << llendl;
	llinfos << "OpenAL vendor: "
		<< ll_safe_string(alGetString(AL_VENDOR)) << llendl;
	llinfos << "OpenAL renderer: "
		<< ll_safe_string(alGetString(AL_RENDERER)) << llendl;

	ALint major = alutGetMajorVersion();
	ALint minor = alutGetMinorVersion();
	llinfos << "ALUT version: " << major << "." << minor << llendl;

	alcGetIntegerv(mDevice, ALC_MAJOR_VERSION, 1, &major);
	alcGetIntegerv(mDevice, ALC_MINOR_VERSION, 1, &minor);
	llinfos << "ALC version: " << major << "." << minor << llendl;

	return true;
}

// virtual
std::string LLAudioEngine_OpenAL::getDriverName(bool verbose)
{
	ALCdevice *device = alcGetContextsDevice(alcGetCurrentContext());
	std::ostringstream version;

	version <<
		"OpenAL";

	if (verbose)
	{
		version <<
			", version " <<
			ll_safe_string(alGetString(AL_VERSION)) <<
			" / " <<
			ll_safe_string(alGetString(AL_VENDOR)) <<
			" / " <<
			ll_safe_string(alGetString(AL_RENDERER));
		
		if (device)
			version <<
				": " <<
				ll_safe_string(alcGetString(device,
				    ALC_DEFAULT_DEVICE_SPECIFIER));
	}

	return version.str();
}

// virtual
void LLAudioEngine_OpenAL::allocateListener()
{
	mListenerp = (LLListener *) new LLListener_OpenAL();
	if (!mListenerp)
	{
		llwarns << "LLAudioEngine_OpenAL::allocateListener() Listener creation failed" << llendl;
	}
}

// virtual
void LLAudioEngine_OpenAL::shutdown()
{
	llinfos << "Entering LLAudioEngine::shutdown()" << llendl;

	LLAudioEngine::shutdown();

	llinfos << "Entering alutExit()" << llendl;
	if (!alutExit())
	{
		llwarns << "LLAudioEngine_OpenAL::shutdown() ALUT shutdown failed: " << alutGetErrorString(alutGetError()) << llendl;
	}
	else
	{
		llinfos << "LLAudioEngine_OpenAL::shutdown() OpenAL successfully shut down" << llendl;
	}

	delete mListenerp;
	mListenerp = NULL;

	ALenum error;

	alcMakeContextCurrent(NULL);
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		llinfos << "AL error: " << convertALErrorToString(error) << ". Could not make current context NULL!" << llendl;
	}

	alcDestroyContext(mContext);
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		llinfos << "AL error: " << convertALErrorToString(error) << ". Could not destroy context!" << llendl;
	}
	else
	{
		mContext = NULL;
	}

    alcCloseDevice(mDevice);
	error = alGetError();
	if (error != AL_NO_ERROR)
	{
		llinfos << "AL error: " << convertALErrorToString(error) << ". Could not close device!" << llendl;
	}
	else
	{
		mDevice = NULL;
	}
}

LLAudioBuffer *LLAudioEngine_OpenAL::createBuffer()
{
	return new LLAudioBufferOpenAL();
}

LLAudioChannel *LLAudioEngine_OpenAL::createChannel()
{
	return new LLAudioChannelOpenAL();
}

void LLAudioEngine_OpenAL::setInternalGain(F32 gain)
{
	//llinfos << "LLAudioEngine_OpenAL::setInternalGain() Gain: " << gain << llendl;
	alListenerf(AL_GAIN, gain);
}

LLAudioChannelOpenAL::LLAudioChannelOpenAL()
	:
	mALSource(AL_NONE),
	mLastSamplePos(0)
{
	alGenSources(1, &mALSource);
}

LLAudioChannelOpenAL::~LLAudioChannelOpenAL()
{
	cleanup();
	if (mALSource != AL_NONE)
	{
		alDeleteSources(1, &mALSource);
		mALSource = AL_NONE;
	}
}

void LLAudioChannelOpenAL::cleanup()
{
	alSourceStop(mALSource);
	alSourcei(mALSource, AL_BUFFER, 0); // need to unset buffer too, or alDeleteBuffers will fail.

	mCurrentBufferp = NULL;
}

void LLAudioChannelOpenAL::play()
{
	if (mALSource == AL_NONE)
	{
		llwarns << "Playing without a mALSource, aborting" << llendl;
		return;
	}

	if (!isPlaying())
	{
		alSourcePlay(mALSource);
		getSource()->setPlayedOnce(true);
	}
}

void LLAudioChannelOpenAL::playSynced(LLAudioChannel *channelp)
{
	if (channelp)
	{
		LLAudioChannelOpenAL *masterchannelp =
			(LLAudioChannelOpenAL*)channelp;
		if (mALSource != AL_NONE &&
		    masterchannelp->mALSource != AL_NONE)
		{
			// we have channels allocated to master and slave
			ALfloat master_offset;
			alGetSourcef(masterchannelp->mALSource, AL_SEC_OFFSET,
				     &master_offset);

			llinfos << "Syncing with master at " << master_offset
				<< "sec" << llendl;
			// *TODO: detect when this fails, maybe use AL_SAMPLE_
			alSourcef(mALSource, AL_SEC_OFFSET, master_offset);
		}
	}
	play();
}

bool LLAudioChannelOpenAL::isPlaying()
{
	if (mALSource != AL_NONE)
	{
		ALint state;
		alGetSourcei(mALSource, AL_SOURCE_STATE, &state);
		if(state == AL_PLAYING)
		{
			return true;
		}
	}
		
	return false;
}

bool LLAudioChannelOpenAL::updateBuffer()
{
	if (LLAudioChannel::updateBuffer())
	{
		// Base class update returned true, which means that we need to actually
		// set up the source for a different buffer.
		LLAudioBufferOpenAL *bufferp = (LLAudioBufferOpenAL *)mCurrentSourcep->getCurrentBuffer();
		ALuint buffer = bufferp->getBuffer();
		alSourcei(mALSource, AL_BUFFER, buffer);
		mLastSamplePos = 0;
	}

	if (mCurrentSourcep)
	{
		alSourcef(mALSource, AL_GAIN,
			  mCurrentSourcep->getGain() * getSecondaryGain());
		alSourcei(mALSource, AL_LOOPING,
			  mCurrentSourcep->isLoop() ? AL_TRUE : AL_FALSE);
		alSourcef(mALSource, AL_ROLLOFF_FACTOR,
			  gAudiop->mListenerp->getRolloffFactor());
	}

	return true;
}


void LLAudioChannelOpenAL::updateLoop()
{
	if (mALSource == AL_NONE)
	{
		return;
	}

	// Hack:  We keep track of whether we looped or not by seeing when the
	// sample position looks like it's going backwards.  Not reliable; may
	// yield false negatives.
	//
	ALint cur_pos;
	alGetSourcei(mALSource, AL_SAMPLE_OFFSET, &cur_pos);
	if (cur_pos < mLastSamplePos)
	{
		mLoopedThisFrame = true;
	}
	mLastSamplePos = cur_pos;
}


void LLAudioChannelOpenAL::update3DPosition()
{
	if(!mCurrentSourcep)
	{
		return;
	}
	if (mCurrentSourcep->isAmbient())
	{
		alSource3f(mALSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mALSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_TRUE);
	} else {
		LLVector3 float_pos;
		float_pos.setVec(mCurrentSourcep->getPositionGlobal());
		alSourcefv(mALSource, AL_POSITION, float_pos.mV);
		alSourcefv(mALSource, AL_VELOCITY, mCurrentSourcep->getVelocity().mV);
		alSourcei (mALSource, AL_SOURCE_RELATIVE, AL_FALSE);
	}

	alSourcef(mALSource, AL_GAIN, mCurrentSourcep->getGain() * getSecondaryGain());
}

LLAudioBufferOpenAL::LLAudioBufferOpenAL()
{
	mALBuffer = AL_NONE;
}

LLAudioBufferOpenAL::~LLAudioBufferOpenAL()
{
	cleanup();
}

void LLAudioBufferOpenAL::cleanup()
{
	if (mALBuffer != AL_NONE)
	{
		alGetError();
		alDeleteBuffers(1, &mALBuffer);

		ALenum error = alGetError();
		if (AL_NO_ERROR != error)
		{
			llwarns << "OpenAL error: " << error << " possible memory leak hit" << llendl;
		}

		mALBuffer = AL_NONE;
	}
}

bool LLAudioBufferOpenAL::loadWAV(const std::string& filename)
{
	cleanup();
	mALBuffer = alutCreateBufferFromFile(filename.c_str());
	if (mALBuffer == AL_NONE)
	{
		ALenum error = alutGetError(); 
		if (gDirUtilp->fileExists(filename))
		{
			llwarns << "LLAudioBufferOpenAL::loadWAV() Error loading "
					<< filename << " " 
					<< convertALErrorToString(error) << ": "
					<< alutGetErrorString(error) 
					<< llendl;
		}
		else
		{
			// It's common for the file to not actually exist.
			lldebugs << "LLAudioBufferOpenAL::loadWAV() Error loading "
					<< filename << " " 
					<< convertALErrorToString(error) << ": "
					<< alutGetErrorString(error) 
					<< llendl;
		}
		return false;
	}

	return true;
}

U32 LLAudioBufferOpenAL::getLength()
{
	if (mALBuffer == AL_NONE)
	{
		return 0;
	}
	ALint length;
	alGetBufferi(mALBuffer, AL_SIZE, &length);
	return length / 2; // convert size in bytes to size in (16-bit) samples
}

// ------------

bool LLAudioEngine_OpenAL::initWind()
{
	ALenum error;
	llinfos << "LLAudioEngine_OpenAL::initWind() start" << llendl;

	mNumEmptyWindALBuffers = MAX_NUM_WIND_BUFFERS;

	alGetError(); /* clear error */
	
	alGenSources(1,&mWindSource);
	
	if ((error=alGetError()) != AL_NO_ERROR)
	{
		llwarns << "LLAudioEngine_OpenAL::initWind() Error creating wind sources: " << convertALErrorToString(error) << llendl;
	}

	mWindGen = new LLWindGen<WIND_SAMPLE_T>;

	mWindBufFreq = mWindGen->getInputSamplingRate();
	mWindBufSamples = llceil(mWindBufFreq * WIND_BUFFER_SIZE_SEC);
	mWindBufBytes = mWindBufSamples * 2 /*stereo*/ * sizeof(WIND_SAMPLE_T);

	mWindBuf = new WIND_SAMPLE_T [mWindBufSamples * 2 /*stereo*/];

	if (mWindBuf == NULL)
	{
		llerrs << "LLAudioEngine_OpenAL::initWind() Error creating wind memory buffer" << llendl;
		return false;
	}

	llinfos << "LLAudioEngine_OpenAL::initWind() done" << llendl;

	return true;
}

void LLAudioEngine_OpenAL::cleanupWind()
{
	llinfos << "LLAudioEngine_OpenAL::cleanupWind()" << llendl;

	if (mWindSource != AL_NONE)
	{
		// detach and delete all outstanding buffers on the wind source
		alSourceStop(mWindSource);
		ALint processed;
		alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
		while (processed--)
		{
			ALuint buffer = AL_NONE;
			alSourceUnqueueBuffers(mWindSource, 1, &buffer);
			alDeleteBuffers(1, &buffer);
		}

		// delete the wind source itself
		alDeleteSources(1, &mWindSource);

		mWindSource = AL_NONE;
	}
	
	delete[] mWindBuf;
	mWindBuf = NULL;

	delete mWindGen;
	mWindGen = NULL;
}

void LLAudioEngine_OpenAL::updateWind(LLVector3 wind_vec, F32 camera_altitude)
{
	LLVector3 wind_pos;
	F64 pitch;
	F64 center_freq;
	ALenum error;
	
	if (!mEnableWind)
		return;
	
	if (!mWindBuf)
		return;
	
	if (mWindUpdateTimer.checkExpirationAndReset(LL_WIND_UPDATE_INTERVAL))
	{
		
		// wind comes in as Linden coordinate (+X = forward, +Y = left, +Z = up)
		// need to convert this to the conventional orientation DS3D and OpenAL use
		// where +X = right, +Y = up, +Z = backwards
		
		wind_vec.setVec(-wind_vec.mV[1], wind_vec.mV[2], -wind_vec.mV[0]);
		
		pitch = 1.0 + mapWindVecToPitch(wind_vec);
		center_freq = 80.0 * pow(pitch,2.5*(mapWindVecToGain(wind_vec)+1.0));
		
		mWindGen->mTargetFreq = (F32)center_freq;
		mWindGen->mTargetGain = (F32)mapWindVecToGain(wind_vec) * mMaxWindGain;
		mWindGen->mTargetPanGainR = (F32)mapWindVecToPan(wind_vec);
		
		alSourcei(mWindSource, AL_LOOPING, AL_FALSE);
		alSource3f(mWindSource, AL_POSITION, 0.0, 0.0, 0.0);
		alSource3f(mWindSource, AL_VELOCITY, 0.0, 0.0, 0.0);
		alSourcef(mWindSource, AL_ROLLOFF_FACTOR, 0.0);
		alSourcei(mWindSource, AL_SOURCE_RELATIVE, AL_TRUE);
	}

	// ok lets make a wind buffer now

	ALint processed, queued, unprocessed;
	alGetSourcei(mWindSource, AL_BUFFERS_PROCESSED, &processed);
	alGetSourcei(mWindSource, AL_BUFFERS_QUEUED, &queued);
	unprocessed = queued - processed;

	// ensure that there are always at least 3x as many filled buffers
	// queued as we managed to empty since last time.
	mNumEmptyWindALBuffers = llmin(mNumEmptyWindALBuffers + processed * 3 - unprocessed, MAX_NUM_WIND_BUFFERS-unprocessed);
	mNumEmptyWindALBuffers = llmax(mNumEmptyWindALBuffers, 0);

	//llinfos << "mNumEmptyWindALBuffers: " << mNumEmptyWindALBuffers	<<" (" << unprocessed << ":" << processed << ")" << llendl;

	while (processed--) // unqueue old buffers
	{
		ALuint buffer;
		ALenum error;
		alGetError(); /* clear error */
		alSourceUnqueueBuffers(mWindSource, 1, &buffer);
		error = alGetError();
		if (error != AL_NO_ERROR)
		{
			llwarns << "LLAudioEngine_OpenAL::updateWind() error swapping (unqueuing) buffers" << llendl;
		}
		else
		{
			alDeleteBuffers(1, &buffer);
		}
	}

	unprocessed += mNumEmptyWindALBuffers;
	while (mNumEmptyWindALBuffers > 0) // fill+queue new buffers
	{
		ALuint buffer;
		alGetError(); /* clear error */
		alGenBuffers(1,&buffer);
		if ((error=alGetError()) != AL_NO_ERROR)
		{
			llwarns << "LLAudioEngine_OpenAL::updateWind() Error creating wind buffer: " << convertALErrorToString(error) << llendl;
			break;
		}

		alBufferData(buffer,
			     AL_FORMAT_STEREO16,
			     mWindGen->windGenerate(mWindBuf,
						    mWindBufSamples),
			     mWindBufBytes,
			     mWindBufFreq);
		error = alGetError();
		if (error != AL_NO_ERROR)
		{
			llwarns << "LLAudioEngine_OpenAL::updateWind() error swapping (bufferdata) buffers" << llendl;
		}
		
		alSourceQueueBuffers(mWindSource, 1, &buffer);
		error = alGetError();
		if (error != AL_NO_ERROR)
		{
			llwarns << "LLAudioEngine_OpenAL::updateWind() error swapping (queuing) buffers" << llendl;
		}

		--mNumEmptyWindALBuffers;
	}

	ALint playing;
	alGetSourcei(mWindSource, AL_SOURCE_STATE, &playing);
	if (playing != AL_PLAYING)
	{
		alSourcePlay(mWindSource);

		lldebugs << "Wind had stopped - probably ran out of buffers - restarting: " 
				<< (unprocessed+mNumEmptyWindALBuffers) << " now queued." 
				<< llendl;
	}
}

