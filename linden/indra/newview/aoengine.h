/*
Anything that handles the actual AO workings that's not UI
*/

#ifndef AO_ENGINE_H
#define AO_ENGINE_H

#include "llmemory.h"
#include "aostate.h"


class AOStandTimer : public LLEventTimer
{
public:
    AOStandTimer();
    ~AOStandTimer();
    virtual BOOL tick();
	virtual void reset();
};



class AOOverride;

class AOEngine : public LLSingleton<AOEngine> 
{
public:
    AOEngine();
	/*virtual*/ ~AOEngine();

	/*virtual*/ void initSingleton();

	static const LLUUID& getAssetIDByName(const std::string& name);

	bool init();
	void run();
	void reset();

	AOOverride* getOverrideFromState(EAOState::State state);

	void addAnim(EAOState::State state, const LLUUID& anim_id, const std::string& anim_name);
	
	// Prefer using UUIDs over names where possible
	bool removeAnim(const LLUUID& anim_id);
	// Prefer using UUIDs over names where possible
	bool removeAnim(const std::string& name);
	bool removeAnims(EAOState::State state);
	bool removeAnims(const LLUUID& sim_anim_id);

	LLUUID getOverride(const LLUUID& sim_anim_id, bool is_starting);

	// returns a user-set override for a sim animation
	LLUUID getOverrideID(const LLUUID& sim_anim_id);
	// returns a user-set override for a specific state
	LLUUID getOverrideID(EAOState::State state);

	bool hasOverride(EAOState::State state);

	void setRandom(EAOState::State state, bool random);

	LLUUID getLastPlayedIDFromState(EAOState::State state);
	void setLastPlayedIDForState(EAOState::State state, const LLUUID& anim_id);

	// Special function for when our underwater state changes. Do not call outside of LLVOAvatar
	void changedUnderwater();

private:

	// map of animations we can use
	std::map<EAOState::State, AOOverride*> mAOList;

	EAOState::State mCurrentState;
	bool mInitialized;
	LLUUID mInvFolderID;
	LLUUID mLastPlayedIDEver;
	LLUUID mLastOverriddenID;

	void cleanupAnims();

	static AOState* sStateList;

public:

	EAOState::State getCurrentState() { return mCurrentState; }
	void setCurrentState(EAOState::State state) { mCurrentState = state; }

	const LLUUID& getLastPlayedIDEver() const { return mLastPlayedIDEver; }
	void setLastPlayedIDEver(const LLUUID& anim_id) { mLastPlayedIDEver = anim_id; }

	const LLUUID& getLastOverriddenID() const { return mLastOverriddenID; }
	void setLastOverriddenID(const LLUUID& anim_id) { mLastOverriddenID = anim_id; }

	const LLUUID& getInvFolderID() const { return mInvFolderID; }

	static AOState* getStateList();

protected:

	static void onNotecardLoadComplete(LLVFS *vfs, const LLUUID& asset_uuid, LLAssetType::EType type, void* user_data, S32 status, LLExtStat ext_status);
};

#endif
