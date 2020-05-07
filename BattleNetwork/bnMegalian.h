#pragma once
#include "bnMegalianIdleState.h"
#include "bnAI.h"
#include "bnCharacter.h"
#include "bnObstacle.h"
#include "bnTextureType.h"
#include "bnMobHealthUI.h"
#include "bnAnimationComponent.h"
#include "bnAudioResourceManager.h"
#include "bnTextureResourceManager.h"
#include "bnDefenseAura.h"
#include <Swoosh/Ease.h>

/*! \brief Megalian enemy is composed of two characters: one deals damage and propogates all damage, and the other controls the whole */
class Megalian : public Character, public AI<Megalian> {
  friend class MegalianIdleState;

private:
  class Head : public Obstacle {
  private:
    int moveCount;
    Battle::Tile* lastTile;
    friend class Megalian;
    double progress;
    DefenseRule* copout;
  public:
    Head(Megalian* belongsTo) : base(belongsTo), Obstacle(belongsTo->GetField(), belongsTo->GetTeam()) {
      SetFloatShoe(true);
      SetTeam(base->GetTeam());
      SharedHitboxDamage(base);
      animation = CreateComponent<AnimationComponent>(this);
      auto baseAnimation = belongsTo->GetFirstComponent<AnimationComponent>();
      auto str = baseAnimation->GetFilePath();
      animation->SetPath(str);
      animation->Load();
      animation->SetAnimation("Head1");
      animation->SetPlaybackSpeed(0); 
      setScale(2.f, 2.f);
      setTexture(TEXTURES.GetTexture(TextureType::MOB_MEGALIAN_ATLAS));
      animation->OnUpdate(0);
      SetLayer(-1); // on top of base
      SetHealth(base->GetHealth());
      SetDirection(Direction::left);
      SetSlideTime(sf::milliseconds(250));
      timer = 0;
      moveCount = 0;
      progress = 0;
      ShareTileSpace(true);
      auto props = GetHitboxProperties();
      props.damage = 30;
      props.flags = Hit::impact | Hit::breaking | Hit::recoil | Hit::flinch;
      props.aggressor = base;
      SetHitboxProperties(props);
      SetName("M. Head");
      lastTile = nullptr;

      DefenseAura::Callback nothing = [](Spell& in, Character& protect) {};
      copout = new DefenseAura(nothing);
    }

    virtual ~Head() { delete copout;  }

    virtual void OnUpdate(float _elapsed) { 
      progress += _elapsed;
      if (!base || !base->GetHealth()) {
        if (int(progress * 1000) % 2 == 0) {
          Hide();
        }
        else {
          Reveal();
        }

        if (base && base->IsDeleted()) {
          Delete();
        }

        return;
      }

      if (GetTile() == base->GetTile() && base->HasAura()) {
        AddDefenseRule(copout);
      }
      else {
        RemoveDefenseRule(copout);
      }

      if (lastTile && lastTile != GetTile()) {
        moveCount++;
        lastTile = GetTile();
      }

      // share HP across both peices
      SetHealth(base->GetHealth());

      auto baseOffset = base->GetFirstComponent<AnimationComponent>()->GetPoint("head");
      auto origin = base->getSprite().getOrigin();

      // transform from sprite space to world space -- scale by 2
      baseOffset = baseOffset - origin;
      baseOffset = sf::Vector2f(2.0f*baseOffset.x, 2.0f*baseOffset.y);

      setPosition(tile->getPosition().x, tile->getPosition().y);
      setPosition(getPosition() + baseOffset + tileOffset);

      tile->AffectEntities(this);

      if (!IsSliding()) {
        timer += _elapsed;

        if (timer > 1 && lastTile == nullptr && GetDirection() == Direction::none && moveCount == 0) {
          Battle::Tile* nextTile = nullptr;

          int x = 0;
          int y = 0;

          while (!nextTile) {
            x = (rand() % 3) + 4;
            y = (rand() % 3) + 1;

            nextTile = GetField()->GetAt(x, y);

            if (nextTile->ContainsEntityType<Megalian>() || nextTile->GetTeam() != GetTeam())
              nextTile = nullptr;
          }

          // teleporting erases our direction information
          auto tele1 = Teleport(x, y);
          AdoptNextTile();
          auto tele2 = base->Teleport(x, y);
          base->AdoptNextTile();
          base->FinishMove();
          SetDirection(Direction::left);

          Logger::Log("tele1: " + std::to_string(tele1) + " tele2: " + std::to_string(tele2));

          lastTile = GetTile();
        }

        if (timer >= 5) {
          static bool playOnce = true;
          auto adjusted = 0;  swoosh::ease::wideParabola(timer - 5.0, 0.1, 1.0);

          SetSlideTime(sf::milliseconds(250 + (adjusted * 500)));

          Logger::Log("timer: " + std::to_string(timer-5.0) + " adjusted: " + std::to_string(adjusted) + " SetSlideTime: " + std::to_string(250 + (adjusted * 500)));

          if(playOnce) {
            AUDIO.Play(AudioType::TOSS_ITEM_LITE);
            playOnce = false;
          }

          if (GetTile() == base->GetTile() && GetDirection() == Direction::right) {
            SetDirection(Direction::none);
            timer = 0;
            animation->SetFrame(1);
            playOnce = true;
            moveCount = 0;
            lastTile = nullptr;
          }
          else {

            SlideToTile(true);
            Move(GetDirection());
            animation->SetFrame(2);

            if(moveCount >= 2) {
              SetDirection(Direction::right);
            }
          }
        }

      }
    }

    const bool OnHit(const Hit::Properties props) {
      return true;
    }
    virtual void OnDelete() { }
    virtual const float GetHeight() const { return 15; }
    virtual bool CanMoveTo(Battle::Tile * next) { return true;  }

    virtual void Attack(Character* e) {
      e->Hit(GetHitboxProperties());
    }

  protected:
    AnimationComponent* animation;
    Megalian* base;
    double timer;

  };

public:
  using DefaultState = MegalianIdleState;

  /**
 * @brief Loads animations and gives itself a turn ID
 */
  Megalian(Rank _rank = Rank::_1);

  /**
   * @brief Removes its turn ID from the list of active mettaurs
   */
  virtual ~Megalian();

  /**
   * @brief Uses AI state to move around. Deletes when health is below zero.
   * @param _elapsed in seconds
   */
  virtual void OnUpdate(float _elapsed);

  /**
   * @brief Takes damage and flashes white
   * @param props
   * @return true if hit, false if missed
   */
  virtual const bool OnHit(const Hit::Properties props);

  virtual void OnDelete();

  /**
   * @brief Get the hit height of this entity
   * @return const float
   */
  virtual const float GetHeight() const;

  const bool HasAura();

  virtual bool CanMoveTo(Battle::Tile* next);

private:

  AnimationComponent* animationComponent;
  float hitHeight; /*!< hit height */
  TextureType textureType;
  Head* head;
};
