#include "bnPlayerNetworkState.h"

#include "../bnPlayer.h"
#include "../bnCardAction.h"
#include "../bnTile.h"
#include "../bnSelectedCardsUI.h"
#include "../bnAudioResourceManager.h"

#include <iostream>

PlayerNetworkState::PlayerNetworkState(NetPlayFlags& netflags) : AIState<Player>(), netflags(netflags)
{
  isChargeHeld = false;
  queuedAction = nullptr;
}


PlayerNetworkState::~PlayerNetworkState()
{
}

void PlayerNetworkState::QueueAction(Player& player)
{
  // peek into the player's queued Action property
  CardAction* action = player.DequeueAction();

  // We already have one action queued, delete the next one
  if (!queuedAction) {
    queuedAction = action;
  }
  else {
    delete action;
  }
}

void PlayerNetworkState::OnEnter(Player& player) {
  player.SetAnimation(PLAYER_IDLE);
}

void PlayerNetworkState::OnUpdate(double _elapsed, Player& player) {
  QueueAction(player);

  // Action controls take priority over movement
  if (player.GetComponentsDerivedFrom<CardAction>().size()) return;

  if (!netflags.isRemoteReady) {
    netflags.remoteCharge = netflags.remoteShoot = netflags.remoteUseSpecial = false;
    netflags.remoteDirection = Direction::none;
    return;
  }

  if (netflags.remoteUseSpecial) {
    player.UseSpecial();
    QueueAction(player);
    netflags.remoteUseSpecial = false;
  }    // queue attack based on input behavior (buster or charge?)
  else if ((!netflags.remoteCharge && isChargeHeld) || netflags.remoteShoot == true ) {
    // This routine is responsible for determining the outcome of the attack
    player.Attack();
    netflags.remoteShoot = false;
    QueueAction(player);
    isChargeHeld = false;
    player.chargeEffect.SetCharging(false);
  }

  // Movement increments are restricted based on anim speed at this time
  if (player.state != PLAYER_IDLE)
    return;

  static Direction direction = Direction::none;
  if (!player.IsTimeFrozen()) {
    direction = netflags.remoteDirection;
    netflags.remoteDirection = Direction::none; // we're moving now
  }

  bool shouldShoot = netflags.remoteCharge && isChargeHeld == false;

#ifdef __ANDROID__
  shouldShoot = Input().Has(PRESSED_A);
#endif

  if (shouldShoot) {
    isChargeHeld = true;

    player.chargeEffect.SetCharging(true);
  }

  if (player.GetFirstComponent<AnimationComponent>()->GetAnimationString() != PLAYER_IDLE || player.IsSliding()) return;

  if (player.PlayerControllerSlideEnabled()) {
    player.SlideToTile(true);
  }

  if (player.Move(direction)) {

    bool moved = player.GetNextTile();

    if (moved) {
      auto onFinish = [&]() {
        player.SetAnimation("PLAYER_MOVED", [p = &player]() {
          p->SetAnimation(PLAYER_IDLE);
          p->FinishMove();
          });

        player.AdoptNextTile();
        direction = Direction::none;
      }; // end lambda
      player.GetFirstComponent<AnimationComponent>()->CancelCallbacks();
      player.SetAnimation(PLAYER_MOVING, onFinish);
    }
  }
  else if (queuedAction && !player.IsSliding()) {
    player.RegisterComponent(queuedAction);
    queuedAction->Execute();
    queuedAction = nullptr;

    player.chargeEffect.SetCharging(false);
    isChargeHeld = false;
  }
}

void PlayerNetworkState::OnLeave(Player& player) {
  /* Navis lose charge when we leave this state */
  player.chargeEffect.SetCharging(false);

  if (auto queuedAction = player.DequeueAction(); queuedAction) {
    delete queuedAction;
  }

  /* Cancel card actions */
  auto actions = player.GetComponentsDerivedFrom<CardAction>();

  for (auto a : actions) {
    a->EndAction();
  }
}
