#pragma once

#include "Swoosh/Ease.h"

#include <SFML/Graphics.hpp>
#include "bnTextureResourceManager.h"
#include "bnAudioResourceManager.h"
#include "bnChipFolder.h"
#include "bnEngine.h"
#include "bnAnimation.h"
#include "bnChipDescriptionTextbox.h"
#include "bnCustEmblem.h"

class ChipSelectionCust {
public:
  struct Bucket {
    Chip* data;
    short state;
  };

private:
  sf::Sprite custSprite;
  sf::Sprite cursorSmall; // animated
  sf::Sprite cursorBig;   // animated
  sf::Sprite chipLock;
  Animation cursorSmallAnimator;
  Animation cursorBigAnimator;
  LayeredDrawable icon;
  LayeredDrawable chipCard;
  LayeredDrawable chipNoData;
  LayeredDrawable chipSendData;
  LayeredDrawable element;
  sf::Shader& greyscale;
  sf::Font* labelFont;
  sf::Font* codeFont;
  sf::Text smCodeLabel;
  sf::Text label;
  CustEmblem emblem;

  int chipCount;
  int selectCount;
  int chipCap;
  int cursorPos;
  int cursorRow;
  bool areChipsReady;
  int perTurn;
  ChipFolder* folder;
  Chip** selectedChips;
  Bucket* queue;
  Bucket** selectQueue;
  ChipDescriptionTextbox chipDescriptionTextbox;

  double frameElapsed;

public:
  ChipSelectionCust(ChipFolder* _folder, int, int perTurn = 5);
  ~ChipSelectionCust();

  // GUI ops
  bool OpenChipDescription();
  bool ContinueChipDescription();
  bool FastForwardChipDescription(double factor);
  bool CloseChipDescription();
  bool ChipDescriptionYes();
  bool ChipDescriptionNo();
  bool CursorUp();
  bool CursorDown();
  bool CursorRight();
  bool CursorLeft();
  bool CursorAction();
  bool CursorCancel();

  bool IsOutOfView();
  bool IsInView();
  bool IsChipDescriptionTextBoxOpen();
  void Move(sf::Vector2f delta);
  const sf::Vector2f GetOffset() const { return custSprite.getPosition() - sf::Vector2f(-custSprite.getTextureRect().width*2.f, 0.f); } // TODO: Get rid. See BattleScene.cpp line 241
  void Draw();
  void Update(float elapsed);

  bool ChipDescriptionConfirmQuestion();

  // Chip ops
  void GetNextChips();
  Chip** GetChips();
  void ClearChips();
  const int GetChipCount();
  void ResetState();
  bool AreChipsReady();
};

