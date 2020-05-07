#include "bnAlphaCore.h"
#include "bnAlphaArm.h"
#include "bnAlphaClawSwipeState.h"
#include "bnAlphaRocketState.h"
#include "bnAlphaElectricState.h"
#include "bnAlphaGunState.h"
#include "bnObstacle.h"
#include "bnMobMoveEffect.h"
#include "bnTile.h"
#include "bnField.h"
#include "bnWave.h"
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnEngine.h"
#include "bnExplodeState.h"
#include "bnDefenseVirusBody.h"
#include "bnHitbox.h"
#include <cmath>

#define RESOURCE_PATH "resources/mobs/alpha/alpha.animation"

AlphaCore::AlphaCore(Rank _rank)
  : BossPatternAI<AlphaCore>(this), Character(_rank) {
  Entity::team = Team::blue;
  totalElapsed = 0;
  coreHP = prevCoreHP = 40;
  coreRegen = 0;
  setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  setScale(2.f, 2.f);

  SetName("Alpha");
  SetHealth(2000);
  SetLayer(1);

  impervious = false;
  shootSuperVulcans = false;

  //Components setup and load
  animationComponent = CreateComponent<AnimationComponent>(this);
  animationComponent->SetPath(RESOURCE_PATH);
  animationComponent->Load();
  animationComponent->SetAnimation("CORE_FULL");
  animationComponent->SetPlaybackMode(Animator::Mode::Loop);
  animationComponent->OnUpdate(0);

  animation = Animation(animationComponent->GetFilePath());
  animation.Load();

  acid = new SpriteProxyNode();
  acid->SetLayer(1);
  acid->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  animation.SetAnimation("ACID");
  animation.Update(0, acid->getSprite());

  head = new SpriteProxyNode();
  head->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  head->SetLayer(-2);
  animation.SetAnimation("HEAD");
  animation.Update(0, head->getSprite());

  side = new SpriteProxyNode();
  side->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  side->SetLayer(-1);
  animation.SetAnimation("SIDE");
  animation.Update(0, side->getSprite());

  leftShoulder = new SpriteProxyNode();
  leftShoulder->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  leftShoulder->SetLayer(0);
  animation.SetAnimation("LEFT_SHOULDER");
  animation.Update(0, leftShoulder->getSprite());

  rightShoulder = new SpriteProxyNode();
  rightShoulder->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  rightShoulder->SetLayer(-3);
  animation.SetAnimation("RIGHT_SHOULDER");
  animation.Update(0, rightShoulder->getSprite());

  rightShoulderShoot= new SpriteProxyNode();
  rightShoulderShoot->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  rightShoulderShoot->SetLayer(-4);

  leftShoulderShoot = new SpriteProxyNode();
  leftShoulderShoot->setTexture(TEXTURES.GetTexture(TextureType::MOB_ALPHA_ATLAS));
  leftShoulderShoot->SetLayer(-4);

  AddNode(acid);
  AddNode(head);
  AddNode(side);
  AddNode(rightShoulder);
  AddNode(leftShoulder);
  rightShoulder->AddNode(rightShoulderShoot);
  leftShoulder->AddNode(leftShoulderShoot);

  leftShoulderShoot->Hide();
  rightShoulderShoot->Hide();

  virusBody = new DefenseVirusBody();
  AddDefenseRule(virusBody);

  defense = new AlphaCoreDefenseRule(coreHP);
  AddDefenseRule(defense);

  // arms
  rightArm = new AlphaArm(nullptr, GetTeam(), AlphaArm::Type::RIGHT_IDLE);
  leftArm = new AlphaArm(nullptr, GetTeam(), AlphaArm::Type::LEFT_IDLE);

  // Setup AI pattern
  AddState<AlphaIdleState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaElectricState>();
  AddState<AlphaIdleState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>(true);
  AddState<AlphaGunState>();
  AddState<AlphaIdleState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaClawSwipeState>();
  AddState<AlphaGunState>();
  AddState<AlphaRocketState>();
}

AlphaCore::~AlphaCore() {
  delete head;
  delete acid, side;
}

void AlphaCore::OnUpdate(float _elapsed) {
  totalElapsed += _elapsed;

  if (leftArm && rightArm) {
    leftArm->SyncElapsedTime(totalElapsed + 1.0f);
    rightArm->SyncElapsedTime(totalElapsed + 1.0f);
  }

  float delta = std::sin(10*totalElapsed)*0.5f;
  head->setPosition(-10, -48 - delta);
  side->setPosition(6, -54 - delta);

  // shoulders lag behind
  delta = std::sin(10 * totalElapsed+1.5f)*0.5f;

  if (shootSuperVulcans) {
    delta = std::sin(50 * totalElapsed + 1.5f);
  }

  leftShoulder->setPosition(-21, -62 - delta);

  if (shootSuperVulcans) {
    delta = -delta;
  }

  rightShoulder->setPosition(2, -57 - delta);

  // keep core exposed if deleted for effect
  // If the core hp is less that initial hp, we need to 
  // 1) regen
  // 2) set the correct animation
  // 3) refresh the sprite

  if (coreHP < 40 && GetHealth() > 0 && !impervious) {
    coreRegen += _elapsed;

    // regen after 4 seconds
    if (coreRegen >= 4) {
      coreRegen = 0;
      coreHP += 20;
    }

    coreHP = std::min(40, coreHP);

    if (prevCoreHP != coreHP) {
      if (prevCoreHP > coreHP) {
      //  coreRegen = 0; // restart the regen timer
      }

      if (coreHP <= 0) {
        animationComponent->SetAnimation("CORE_EXPOSED");
        animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }
      else if (coreHP <= 20) {
        animationComponent->SetAnimation("CORE_HALF");
        animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }
      else {
        animationComponent->SetAnimation("CORE_FULL");
        animationComponent->SetPlaybackMode(Animator::Mode::Loop);
      }

      animationComponent->OnUpdate(0);
    }
  }

  prevCoreHP = coreHP;

  setPosition(tile->getPosition().x + tileOffset.x, tile->getPosition().y + tileOffset.y);
  hitHeight = getLocalBounds().height;

  animation.SetAnimation("ACID");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, acid->getSprite());

  animation.SetAnimation("HEAD");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, head->getSprite());

  animation.SetAnimation("SIDE");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, side->getSprite());

  if (shootSuperVulcans) {
    animation.SetAnimation("SUPER_VULCAN");
    animation << Animator::Mode::Loop;
    animation.Update(totalElapsed, leftShoulderShoot->getSprite());

    animation.SetAnimation("SUPER_VULCAN");
    animation << Animator::Mode::Loop;
    animation.Update(totalElapsed+0.04f, rightShoulderShoot->getSprite());
  }

  BossPatternAI<AlphaCore>::Update(_elapsed);
}

const bool AlphaCore::OnHit(const Hit::Properties props) {
    return true;
}

const float AlphaCore::GetHeight() const {
  return (float)hitHeight;
}

void AlphaCore::OnSpawn(Battle::Tile & start)
{
    RevealLeftArm();
    RevealRightArm();

    // Block player from stealing rows
    Battle::Tile* block = GetField()->GetAt(4, 1);
    block->ReserveEntityByID(GetID());

    block = GetField()->GetAt(4, 2);
    block->ReserveEntityByID(GetID());

    block = GetField()->GetAt(4, 3);
    block->ReserveEntityByID(GetID());
}

void AlphaCore::OnDelete() {
  if (virusBody) {
    RemoveDefenseRule(virusBody);
    delete virusBody;
    virusBody = nullptr;
  }

  Logger::Logf("AlphaCore::OnDelete()");

  leftArm? leftArm->Delete() : 0;
  rightArm? rightArm->Delete() : 0;

  leftArm = nullptr;
  rightArm = nullptr;

  AUDIO.StopStream();

  // Explode if health depleted
  InterruptState<ExplodeState<AlphaCore>>(15, 0.8);
}

void AlphaCore::OpenShoulderGuns()
{
  animation.SetAnimation("SUPER_VULCAN");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed, leftShoulderShoot->getSprite());

  animation.SetAnimation("SUPER_VULCAN");
  animation << Animator::Mode::Loop;
  animation.Update(totalElapsed + 0.04f, rightShoulderShoot->getSprite());

  animation.SetAnimation("LEFT_SHOULDER");
  animation.SetFrame(2, leftShoulder->getSprite());

  // TODO: WHY CANT MY NODES JUST LINK UP TO THE POINTS?
  auto bounds = leftShoulder->getLocalBounds();
  auto offset = animation.GetPoint("SHOOT") - sf::Vector2f(bounds.left, bounds.top);

  leftShoulderShoot->setPosition(-offset.x, 0);

  animation.SetAnimation("RIGHT_SHOULDER");
  animation.SetFrame(2, rightShoulder->getSprite());

  bounds = rightShoulder->getLocalBounds();
  offset = animation.GetPoint("SHOOT") - sf::Vector2f(bounds.left, bounds.top);

  rightShoulderShoot->setPosition(-offset.x+10.0f, 5.0f);
}

void AlphaCore::CloseShoulderGuns()
{
  animation.SetAnimation("LEFT_SHOULDER");
  animation.SetFrame(1, leftShoulder->getSprite());

  animation.SetAnimation("RIGHT_SHOULDER");
  animation.SetFrame(1, rightShoulder->getSprite());

  leftShoulderShoot->Hide();
  rightShoulderShoot->Hide();
  shootSuperVulcans = false;
}

void AlphaCore::HideLeftArm()
{
  if (!leftArm) return;

  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, leftArm->GetTile()->GetX(), leftArm->GetTile()->GetY());
  leftArm->GetTile()->ReserveEntityByID(leftArm->GetID());
  leftArm->GetTile()->RemoveEntityByID(leftArm->GetID());
}

void AlphaCore::RevealLeftArm()
{
  if (!leftArm) return;

  GetField()->AddEntity((*leftArm), GetTile()->GetX(), GetTile()->GetY() + 1);
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, GetTile()->GetX(), GetTile()->GetY() + 1);
}

void AlphaCore::HideRightArm()
{
  if (!rightArm) return;

  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, rightArm->GetTile()->GetX(), rightArm->GetTile()->GetY());
  rightArm->GetTile()->ReserveEntityByID(rightArm->GetID());
  rightArm->GetTile()->RemoveEntityByID(rightArm->GetID());
}

void AlphaCore::RevealRightArm()
{
  if (!rightArm) return;

  GetField()->AddEntity((*rightArm), GetTile()->GetX() - 1, GetTile()->GetY() - 1);
  auto fx = new MobMoveEffect(GetField());
  GetField()->AddEntity(*fx, GetTile()->GetX() - 1, GetTile()->GetY() - 1);
}

void AlphaCore::EnableImpervious(bool impervious)
{
  AlphaCore::impervious = impervious;
  coreHP = 40;
}

void AlphaCore::ShootSuperVulcans()
{
  shootSuperVulcans = true;
  leftShoulderShoot->Reveal();
  rightShoulderShoot->Reveal();
}

AlphaCore::AlphaCoreDefenseRule::AlphaCoreDefenseRule(int& alphaCoreHP) : DefenseRule(Priority(0)), alphaCoreHP(alphaCoreHP) {}
AlphaCore::AlphaCoreDefenseRule::~AlphaCoreDefenseRule() { }

const bool AlphaCore::AlphaCoreDefenseRule::CanBlock(DefenseFrameStateArbiter& arbiter, Spell& in, Character& owner) {
  AlphaCore* alpha = static_cast<AlphaCore*>(&owner);
  if (alpha->impervious) {  
    arbiter.BlockDamage();
    arbiter.BlockImpact();
    return true; 
  }

  alphaCoreHP -= in.GetHitboxProperties().damage;
  alphaCoreHP = std::max(0, alphaCoreHP);

  // Combat damage happens in real time, however during TFC the core
  // should protect Alpha until it is able to changed to its exposed state
  return (alpha->animationComponent->GetAnimationString() != "CORE_EXPOSED");
}

Hit::Properties & AlphaCore::AlphaCoreDefenseRule::FilterStatuses(Hit::Properties& statuses)
{
  statuses.flags &= ~Hit::flinch;
  statuses.flags &= ~Hit::recoil;
  statuses.flags &= ~Hit::drag;
  return statuses;
}
