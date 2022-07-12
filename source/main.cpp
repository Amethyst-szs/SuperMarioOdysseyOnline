#include "main.hpp"
#include <cmath>
#include <math.h>
#include "al/factory/ActorFactoryEntries100.h"
#include "gfx/seadColor.h"
#include "math/seadMathCalcCommon.h"
#include "server/Client.hpp"
#include "puppets/PuppetInfo.h"
#include "actors/PuppetActor.h"
#include "al/LiveActor/LiveActor.h"
#include "al/util.hpp"
#include "al/util/AudioUtil.h"
#include "al/util/CameraUtil.h"
#include "al/util/ControllerUtil.h"
#include "al/util/LiveActorUtil.h"
#include "al/util/NerveUtil.h"
#include "debugMenu.hpp"
#include "game/GameData/GameDataFunction.h"
#include "game/HakoniwaSequence/HakoniwaSequence.h"
#include "game/Player/PlayerFunction.h"
#include "game/StageScene/StageScene.h"
#include "helpers.hpp"
#include "layouts/HideAndSeekIcon.h"
#include "logger.hpp"
#include "rs/util.hpp"
#include "server/gamemode/GameModeBase.hpp"
#include "server/HideAndSeekMode.hpp"
#include "timeWarp.h"

static int pInfSendTimer = 0;
static int gameInfSendTimer = 0;

static int debugCheckFrame = 0;

void updatePlayerInfo(GameDataHolderAccessor holder, PlayerActorHakoniwa *p1) {
    if(pInfSendTimer >= 3) {
        Client::sendPlayerInfPacket(p1);

        Client::sendHackCapInfPacket(p1->mHackCap);

        Client::sendCaptureInfPacket(p1);

        pInfSendTimer = 0;
    }

    if (gameInfSendTimer >= 60) {
        Client::sendGameInfPacket(p1, holder);

        gameInfSendTimer = 0;
    }

    pInfSendTimer++;
    gameInfSendTimer++;
}

// ------------- Hooks -------------

int debugPuppetIndex = 0;
int debugCaptureIndex = 0;
static int pageIndex = 0;

static const int maxPages = 3;

void drawMainHook(HakoniwaSequence* curSequence, sead::Viewport* viewport, sead::DrawContext* drawContext)
{

    // sead::FrameBuffer *frameBuffer;
    // __asm ("MOV %[result], X21" : [result] "=r" (frameBuffer));

    // if(Application::sInstance->mFramework) {
    //     Application::sInstance->mFramework->mGpuPerf->drawResult((agl::DrawContext *)drawContext, frameBuffer);
    // }

    Time::calcTime(); // this needs to be ran every frame, so running it here works
    TimeContainer& container = getTimeContainer();

    al::Scene* curScene = curSequence->curScene;
    sead::PrimitiveRenderer* renderer = sead::PrimitiveRenderer::instance();

    if (curScene && isInGame && container.isSceneActive() && container.getTimeArraySize() > 1) {
        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = al::getProjectionSead(curScene, 0);
        renderer->setDrawContext(drawContext);
        renderer->setCamera(*cam);
        renderer->setProjection(*projection);
        renderer->begin();
        renderer->setModelMatrix(sead::Matrix34f::ident);
        for (int i = 0; i < container.getTimeArraySize(); i++) {
            TimeFrame* frame = container.getTimeFrame(i);
            renderer->drawSphere4x8(container.calcDotTrans(frame->position, i),
                sead::MathCalcCommon<int>::min((container.getTimeArraySize()-i)/2, 9),
                container.calcColorFrame(frame->colorFrame, i));
        }
        renderer->end();
    }

    if (!debugMode) {
        al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
        return;
    }

    int dispWidth = al::getLayoutDisplayWidth();
    int dispHeight = al::getLayoutDisplayHeight();

    gTextWriter->mViewport = viewport;

    gTextWriter->mColor = sead::Color4f(1.f, 1.f, 1.f, 0.8f);

    drawBackground((agl::DrawContext*)drawContext);

    gTextWriter->beginDraw();
    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, 10.f));

    gTextWriter->printf("FPS: %d\n", static_cast<int>(round(Application::sInstance->mFramework->calcFps())));

    gTextWriter->setCursorFromTopLeft(sead::Vector2f(10.f, (dispHeight / 3) + 30.f));
    gTextWriter->setScaleFromFontHeight(20.f);

    gTextWriter->printf("Client Socket Connection Status: %s\n", Client::instance()->mSocket->getStateChar());
    gTextWriter->printf("nn::socket::GetLastErrno: 0x%x\n", Client::instance()->mSocket->socket_errno);
    gTextWriter->printf("Packet Queue Length: %d\n", Client::instance()->mSocket->mPacketQueue.size());
    gTextWriter->printf("Total Connected Players: %d\n", Client::getConnectCount() + 1);

    if (curScene && isInGame) {

        sead::LookAtCamera* cam = al::getLookAtCamera(curScene, 0);
        sead::Projection* projection = al::getProjectionSead(curScene, 0);

        PlayerActorHakoniwa* p1 = rs::getPlayerActor(curScene);

        PuppetActor* curPuppet = Client::getPuppet(debugPuppetIndex);

        PuppetActor* debugPuppet = Client::getDebugPuppet();

        if (debugPuppet) {
            curPuppet = debugPuppet;
        }

        renderer->setDrawContext(drawContext);
        renderer->setCamera(*cam);
        renderer->setProjection(*projection);

        gTextWriter->printf("----------- Page %d ------------\n", pageIndex);
        switch (pageIndex) {
        case 0: {
            // PuppetActor *curPuppet = Client::getDebugPuppet();

            if (curPuppet) {

                al::LiveActor* curModel = curPuppet->getCurrentModel();

                PuppetInfo* curPupInfo = curPuppet->getInfo();

                // al::LiveActor *curCapture = curPuppet->getCapture(debugCaptureIndex);

                    gTextWriter->printf("Puppet Index: %d\n", debugPuppetIndex);
                    gTextWriter->printf("Player Name: %s\n", curPupInfo->puppetName);
                    gTextWriter->printf("Connection Status: %s\n", curPupInfo->isConnected ? "Online" : "Offline");
                    gTextWriter->printf("Is in Same Stage: %s\n", curPupInfo->isInSameStage ? "True" : "False");
                    gTextWriter->printf("Is in Capture: %s\n", curPupInfo->isCaptured ? "True" : "False");
                    gTextWriter->printf("Puppet Stage: %s\n", curPupInfo->stageName);
                    gTextWriter->printf("Puppet Scenario: %u\n", curPupInfo->scenarioNo);
                    gTextWriter->printf("Puppet Costume: H: %s B: %s\n", curPupInfo->costumeHead, curPupInfo->costumeBody);
                    //gTextWriter->printf("Packet Coords:\nX: %f\nY: %f\nZ: %f\n", curPupInfo->playerPos.x, curPupInfo->playerPos.y, curPupInfo->playerPos.z);
                    // if (curModel) {
                    //     sead::Vector3f* pupPos = al::getTrans(curModel);
                    //     gTextWriter->printf("In-Game Coords:\nX: %f\nY: %f\nZ: %f\n", pupPos->x, pupPos->y, pupPos->z);
                    // }

                if (curPupInfo->isCaptured) {
                    gTextWriter->printf("Current Capture: %s\n", curPupInfo->curHack);
                    gTextWriter->printf("Current Packet Animation: %s\n", curPupInfo->curAnimStr);
                    gTextWriter->printf("Animation Index: %d\n", curPupInfo->curAnim);
                } else {
                    gTextWriter->printf("Current Packet Animation: %s\n", curPupInfo->curAnimStr);
                    gTextWriter->printf("Animation Index: %d\n", curPupInfo->curAnim);
                    if (curModel) {
                        gTextWriter->printf("Current Animation: %s\n", al::getActionName(curModel));
                    }
                }
            }
        } break;
        case 1: {
            PuppetActor* debugPuppet = Client::getDebugPuppet();
            PuppetInfo* debugInfo = Client::getDebugPuppetInfo();

            if (debugPuppet && debugInfo) {

                    al::LiveActor *curModel = debugPuppet->getCurrentModel();

                    gTextWriter->printf("Is Debug Puppet Tagged: %s\n", BTOC(debugInfo->isIt));

                // gTextWriter->printf("Is Nametag Visible: %s\n", BTOC(debugPuppet->mNameTag->isVisible()));
                // gTextWriter->printf("Is Nametag Alive: %s\n", BTOC(debugPuppet->mNameTag->mIsAlive));
                // gTextWriter->printf("Nametag Normalized Dist: %f\n", debugPuppet->mNameTag->mNormalizedDist);
                // gTextWriter->printf("Nametag State: %s\n", debugPuppet->mNameTag->getCurrentState());
                // gTextWriter->printf("Is Current Model Clipped: %s\n",
                //     BTOC(al::isClipped(curModel)));
                // gTextWriter->printf("Is Debug Puppet Tagged: %s\n", BTOC(debugInfo->isIt));
            }
        } break;
        case 2: {
            al::PlayerHolder* pHolder = al::getScenePlayerHolder(curScene);
            PlayerActorHakoniwa* p1 = pHolder->tryGetPlayer(0);

            if (p1->mHackKeeper && p1->mHackKeeper->currentHackActor) {

                al::LiveActor* curHack = p1->mHackKeeper->currentHackActor;

                gTextWriter->printf("Current Hack Animation: %s\n", al::getActionName(curHack));
                gTextWriter->printf("Current Hack Name: %s\n",
                    p1->mHackKeeper->getCurrentHackName());
                sead::Quatf captureRot = curHack->mPoseKeeper->getQuat();
                gTextWriter->printf("Current Hack Rot: %f %f %f %f\n", captureRot.x,
                    captureRot.y, captureRot.z, captureRot.w);
                sead::Quatf calcRot;
                al::calcQuat(&calcRot, curHack);
                gTextWriter->printf("Calc Hack Rot: %f %f %f %f\n", calcRot.x,
                    calcRot.y, calcRot.z, calcRot.w);
            } else {
                gTextWriter->printf("Cur Action: %s\n", p1->mPlayerAnimator->mAnimFrameCtrl->getActionName());
                gTextWriter->printf("Cur Sub Action: %s\n", p1->mPlayerAnimator->curSubAnim.cstr());
                gTextWriter->printf("Is Cappy Flying? %s\n", BTOC(p1->mHackCap->isFlying()));
                if (p1->mHackCap->isFlying()) {
                    gTextWriter->printf("Cappy Action: %s\n", al::getActionName(p1->mHackCap));
                    sead::Vector3f* capTrans = al::getTransPtr(p1->mHackCap);
                    sead::Vector3f* capRot = &p1->mHackCap->mJointKeeper->mJointRot;
                    gTextWriter->printf("Cap Coords:\nX: %f\nY: %f\nZ: %f\n", capTrans->x, capTrans->y, capTrans->z);
                    gTextWriter->printf("Cap Rot:\nX: %f\nY: %f\nZ: %f\n", capRot->x, capRot->y, capRot->z);
                    gTextWriter->printf("Cap Skew: %f\n", p1->mHackCap->mJointKeeper->mSkew);
                }
            }
            break;
        }
        default:
            break;
        }

        float curColorFrame = container.getColorFrame();
        al::LiveActor* hack = p1->mHackKeeper->currentHackActor;

        const char* captureName;
        if(hack) captureName = p1->mHackKeeper->getCurrentHackName();

        sead::Color4f curColor = container.calcColorFrame(curColorFrame, -1);
        TimeFrame* curFrame = container.getTimeFrame(debugCheckFrame);

        gTextWriter->setCursorFromTopLeft(sead::Vector2f((dispWidth / 5.f) * 3.f + 15.f, (dispHeight / 3.f) + 30.f));
        gTextWriter->setScaleFromFontHeight(20.f);
        if(al::isPadHoldL(-1) && container.oxygen){
            gTextWriter->printf("Oxygen Header Data:\n");
            gTextWriter->printf("Visual Percentage: %f\n", container.oxygen->mPercentage);
            gTextWriter->printf("Oxygen Frames: %i\n", container.oxygen->mOxygenFrames);
            gTextWriter->printf("Damage Frames: %i\n", container.oxygen->mDamageFrames);
            gTextWriter->printf("Percentage Delay: %i\n", container.oxygen->mPercentageDelay);
            gTextWriter->printf("Oxygen Target: %i\n", container.oxygen->mOxygenTarget);
            gTextWriter->printf("Recovery Frames: %i\n", container.oxygen->mRecoveryFrames);
            gTextWriter->printf("Damage Target: %i\n\n", container.oxygen->mDamageTarget);
        } else {
            gTextWriter->printf("Is Rewind: %s\n", container.isRewind() ? "True" : "False");
            gTextWriter->printf("Is Cooldown: %s\n", container.isOnCooldown() ? "True" : "False");
            gTextWriter->printf("Cooldown Charge: %f\n", container.getCooldownTimer());
            gTextWriter->printf("Rewind Delay: %i\n", container.getRewindDelay());
            gTextWriter->printf("Filter ID: %i\n", al::getPostProcessingFilterPresetId(curScene));
            if(hack) gTextWriter->printf("Current Capture Name: %s\n", captureName);
            gTextWriter->printf("Color Frame: %f\n", curColorFrame);
            gTextWriter->printf("Color R: %f\n", curColor.r);
            gTextWriter->printf("Color G: %f\n", curColor.g);
            gTextWriter->printf("Color B: %f\n", curColor.b);
            gTextWriter->printf("Array Size: %i\n", container.getTimeArraySize());
            gTextWriter->printf("\nFrame Data #%i:\n-----------------\n", debugCheckFrame);
            if (curFrame){
                gTextWriter->printf("Animation: %s\n", curFrame->playerFrame.action.cstr());
                gTextWriter->printf("Current Animation Already This: %s\n", curFrame->playerFrame.action.isEqual(p1->mPlayerAnimator->curAnim) ? "True" : "False");
                gTextWriter->printf("Animation Frame: %f\n", curFrame->playerFrame.actionFrame);
                gTextWriter->printf("Is Cap Flying: %s\n", curFrame->capFrame.isFlying ? "True" : "False");
                gTextWriter->printf("Position X: %f\n", curFrame->capFrame.position.x);
                gTextWriter->printf("Position Y: %f\n", curFrame->capFrame.position.y);
                gTextWriter->printf("Position Z: %f\n", curFrame->capFrame.position.z);
            } else {
                gTextWriter->printf("Array is empty\n");
            }
        }

        // Frame scrolling
        if (al::isPadHoldL(-1) && al::isPadTriggerRight(-1) && debugCheckFrame < container.getTimeArraySize())
            debugCheckFrame++;
        if (al::isPadHoldL(-1) && al::isPadTriggerLeft(-1) && debugCheckFrame > 0)
            debugCheckFrame--;

        // Frame fast jumping
        if (al::isPadHoldL(-1) && al::isPadTriggerUp(-1) && debugCheckFrame < container.getTimeArraySize() - 10)
            debugCheckFrame += 10;
        if (al::isPadHoldL(-1) && al::isPadTriggerUp(-1) && debugCheckFrame > container.getTimeArraySize())
            debugCheckFrame = 0;
        
        //Rewind speed edit
        if (al::isPadHoldZR(-1) && al::isPadTriggerRight(-1))
            container.setRewindDelay(1);
        if (al::isPadHoldZR(-1) && al::isPadTriggerLeft(-1))
            container.setRewindDelay(-1);

        renderer->begin();

        // sead::Matrix34f mat = sead::Matrix34f::ident;
        // mat.setBase(3, sead::Vector3f::zero); // Sets the position of the matrix.
        //  For cubes, you need to put this at the location.
        //  For spheres, you can leave this at 0 0 0 since you set it in its draw function.
        renderer->setModelMatrix(sead::Matrix34f::ident);

        if (curPuppet) {
            renderer->drawSphere4x8(curPuppet->getInfo()->playerPos, 20, sead::Color4f(1.f, 0.f, 0.f, 0.25f));
            renderer->drawSphere4x8(al::getTrans(curPuppet), 20, sead::Color4f(0.f, 0.f, 1.f, 0.25f));
        }

        renderer->end();

        isInGame = false;
    }

    gTextWriter->endDraw();

    al::executeDraw(curSequence->mLytKit, "２Ｄバック（メイン画面）");
}

void sendShinePacket(GameDataHolderWriter thisPtr, Shine* curShine) {

    if (!curShine->isGot()) {
        Client::sendShineCollectPacket(curShine->shineId);
    }

    GameDataFunction::setGotShine(thisPtr, curShine->curShineInfo);
}

bool sceneKillHook(GameDataHolderAccessor value)
{
    getTimeContainer().setInactiveTimer(25);

    return GameDataFunction::isMissEndPrevStageForSceneDead(value);
}

void stageInitHook(al::ActorInitInfo *info, StageScene *curScene, al::PlacementInfo const *placement, al::LayoutInitInfo const *lytInfo, al::ActorFactory const *factory, al::SceneMsgCtrl *sceneMsgCtrl, al::GameDataHolderBase *dataHolder) {

    al::initActorInitInfo(info, curScene, placement, lytInfo, factory, sceneMsgCtrl,
                          dataHolder);

    Client::clearArrays();

    Client::setSceneInfo(*info, curScene);

    if (Client::getServerMode() != NONE) {
        GameModeInitInfo initModeInfo(info, curScene);

        Client::initMode(initModeInfo);
    }

    Client::sendGameInfPacket(info->mActorSceneInfo.mSceneObjHolder);
    
    getTimeContainer().setTimeFramesEmpty();
}

PlayerCostumeInfo *setPlayerModel(al::LiveActor *player, const al::ActorInitInfo &initInfo, const char *bodyModel, const char *capModel, al::AudioKeeper *keeper, bool isCloset) {
    Client::sendCostumeInfPacket(bodyModel, capModel);
    return PlayerFunction::initMarioModelActor(player, initInfo, bodyModel, capModel, keeper, isCloset);
}

al::SequenceInitInfo* initInfo;

ulong constructHook() {  // hook for constructing anything we need to globally be accesible

    __asm("STR X21, [X19,#0x208]"); // stores WorldResourceLoader into HakoniwaSequence

    __asm("MOV %[result], X20"
          : [result] "=r"(
              initInfo));  // Save our scenes init info to a gloabl ptr so we can access it later

    Client::createInstance(al::getCurrentHeap());

    return 0x20;
}

bool threadInit(HakoniwaSequence *mainSeq) {  // hook for initializing client class

    al::LayoutInitInfo lytInfo = al::LayoutInitInfo();

    al::initLayoutInitInfo(&lytInfo, mainSeq->mLytKit, 0, mainSeq->mAudioDirector, initInfo->mSystemInfo->mLayoutSys, initInfo->mSystemInfo->mMessageSys, initInfo->mSystemInfo->mGamePadSys);

    Client::instance()->init(lytInfo, mainSeq->mGameDataHolder);

    return GameDataFunction::isPlayDemoOpening(mainSeq->mGameDataHolder);
}

bool hakoniwaSequenceHook(HakoniwaSequence* sequence) {
    StageScene* stageScene = (StageScene*)sequence->curScene;
    getTimeContainer().stageSceneRef = stageScene;

    static bool isCameraActive = false;

    bool isFirstStep = al::isFirstStep(sequence);
    if(isFirstStep) al::validatePostProcessingFilter(stageScene);

    al::PlayerHolder *pHolder = al::getScenePlayerHolder(stageScene);
    PlayerActorHakoniwa* p1 = (PlayerActorHakoniwa*)al::tryGetPlayerActor(pHolder, 0);

    if (isFirstStep) {
        Client::tryRestartCurrentMode();
    }

    isInGame = !stageScene->isPause();

    Client::setGameActive(!stageScene->isPause());
    Client::setStageInfo(stageScene->mHolder);

    Client::updateStates();

    if (Client::isNeedUpdateShines()) {
        Client::updateShines();
    }

    updatePlayerInfo(stageScene->mHolder, p1);

    static bool isDisableMusic = false;

    if (al::isPadHoldZR(-1)) {
        if (al::isPadTriggerUp(-1)) debugMode = !debugMode;
        if (al::isPadTriggerLeft(-1)) pageIndex--;
        if (al::isPadTriggerRight(-1)) pageIndex++;
        if(pageIndex < 0) {
            pageIndex = maxPages - 1;
        }
        if(pageIndex >= maxPages) pageIndex = 0;

    } else if (al::isPadHoldZL(-1)) {

        if (debugMode) {
            if (al::isPadTriggerLeft(-1)) debugPuppetIndex--;
            if (al::isPadTriggerRight(-1)) debugPuppetIndex++;

            if(debugPuppetIndex < 0) {
                debugPuppetIndex = Client::getMaxPlayerCount() - 2;
            }
            if (debugPuppetIndex >= Client::getMaxPlayerCount() - 1)
                debugPuppetIndex = 0;
        }

    } else if (al::isPadHoldL(-1)) {
        if (al::isPadTriggerLeft(-1)) Client::toggleCurrentMode();
        if (al::isPadTriggerRight(-1)) {
            if (debugMode) {
                PuppetInfo *debugPuppet = Client::getDebugPuppetInfo();
                if (debugPuppet) {
                    debugPuppet->playerPos = al::getTrans(p1);
                    al::calcQuat(&debugPuppet->playerRot, p1);
                    const char *hackName = p1->mHackKeeper->getCurrentHackName();
                    debugPuppet->isCaptured = hackName != nullptr;
                    if (debugPuppet->isCaptured) {
                        strcpy(debugPuppet->curHack, hackName);
                    } else {
                        strcpy(debugPuppet->curHack, "");
                    }
                }
            }
        }
        if (al::isPadTriggerUp(-1)) {
            if (debugMode) {
                PuppetActor* debugPuppet = Client::getDebugPuppet();
                if (debugPuppet) {
                    PuppetInfo *info = debugPuppet->getInfo();
                    // info->isIt = !info->isIt;

                    debugPuppet->emitJoinEffect();
                    
                }
            } else {
                isDisableMusic = !isDisableMusic;
            }
        }
    }

    if (isDisableMusic) {
        if (al::isPlayingBgm(stageScene)) {
            al::stopAllBgm(stageScene, 0);
        }
    }

    // Time warp code added here
    if(!stageScene->isPause()) getTimeContainer().updateTimeStates(p1);

    return isFirstStep;

}

void seadPrintHook(const char *fmt, ...)
{
    va_list args;
	va_start(args, fmt);

    Logger::log(fmt, args);

    va_end(args);
}