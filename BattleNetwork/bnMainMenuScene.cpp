#include <Swoosh/ActivityController.h>
#include "bnWebClientMananger.h"

#include "Android/bnTouchArea.h"

#include "bnMainMenuScene.h"
#include "Segues/DiamondTileSwipe.h"
#include "bnCardFolderCollection.h"

#include "Segues/PushIn.h"
#include "Segues/Checkerboard.h"
#include "Segues/PixelateBlackWashFade.h"

using sf::RenderWindow;
using sf::VideoMode;
using sf::Clock;
using sf::Event;
using sf::Font;

MainMenuScene::MainMenuScene(swoosh::ActivityController& controller) :
  camera(ENGINE.GetView()),
  lastIsConnectedState(false),
  swoosh::Activity(&controller)
{
    // When we reach the menu scene we need to load the player information
    // before proceeding to next sub menus
    //data = CardFolderCollection::ReadFromFile("resources/database/folders.txt");

    webAccountIcon.setTexture(LOAD_TEXTURE(WEBACCOUNT_STATUS));
    webAccountIcon.setScale(2.f, 2.f);
    webAccountIcon.setPosition((getController().getVirtualWindowSize().x-96.0f), getController().getVirtualWindowSize().y - 34.0f);
    webAccountAnimator = Animation("resources/ui/webaccount_icon.animation");
    webAccountAnimator.Load();
    webAccountAnimator.SetAnimation("NO_CONNECTION");

    // Draws the scrolling background
    bg = new LanBackground();

    // Generate an infinite map with a branch depth of 3, column size of 10
    // and tile dimensions 47x24
    map = new Overworld::InfiniteMap(3, 10, 47, 24);
  
    // Share the camera
    map->SetCamera(&camera);

    // Show the HUD
    showHUD = true;

    // Selection input delays
    maxSelectInputCooldown = 0.5; // half of a second
    selectInputCooldown = maxSelectInputCooldown;

    // ui sprite maps
    ui.setTexture(LOAD_TEXTURE(MAIN_MENU_UI));
    ui.setScale(2.f, 2.f);
    uiAnimator = Animation("resources/ui/main_menu_ui.animation");
    uiAnimator.Reload();

    // Keep track of selected navi
    currentNavi = 0;

    owNavi.setTexture(LOAD_TEXTURE(NAVI_MEGAMAN_ATLAS));
    owNavi.setScale(2.f, 2.f);
    owNavi.setPosition(0, 0.f);
    naviAnimator = Animation("resources/navis/megaman/megaman.animation");
    naviAnimator.Reload();
    naviAnimator.SetAnimation("PLAYER_OW_RD");
    naviAnimator << Animator::Mode::Loop;

    // Share the navi sprite
    // Map will transform navi's ortho position into isometric position
    map->AddSprite(&owNavi);

    overlay.setTexture(LOAD_TEXTURE(MAIN_MENU));
    overlay.setScale(2.f, 2.f);

    ow.setTexture(LOAD_TEXTURE(MAIN_MENU_OW));
    ow.setScale(2.f, 2.f);

    gotoNextScene = true;

    menuSelectionIndex = lastMenuSelectionIndex = 0;
}

void MainMenuScene::onStart() {
  // Stop any music already playing
  AUDIO.StopStream();
  AUDIO.Stream("resources/loops/loop_overworld.ogg", false);
  
  // Set the camera back to ours
  ENGINE.SetCamera(camera);

  Logger::Log("Fetching account data...");

  accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();

  Logger::Log("waiting for server...");

#ifdef __ANDROID__
  StartupTouchControls();
#endif

  gotoNextScene = false;
}

void MainMenuScene::onUpdate(double elapsed) {
    #ifdef __ANDROID__
    if(gotoNextScene)
        return; // keep the screen looking the same when we come back
    #endif

    if (accountCommandResponse.valid() && is_ready(accountCommandResponse)) {
        try {
            const WebAccounts::AccountState& account = accountCommandResponse.get();
            Logger::Logf("You have %i folders on your account", account.folders.size());
            WEBCLIENT.CacheTextureData(account);
            data = CardFolderCollection::ReadFromWebAccount(account);
        }
        catch (const std::runtime_error& e) {
            Logger::Logf("Could not fetch account.\nError: %s", e.what());
        }
    }

  // update the web connectivity icon
  bool currentConnectivity = WEBCLIENT.IsConnectedToWebServer();
  if (currentConnectivity != lastIsConnectedState) {
    if (WEBCLIENT.IsConnectedToWebServer()) {
        webAccountAnimator.SetAnimation("OK_CONNECTION");
    }
    else {
        webAccountAnimator.SetAnimation("NO_CONNECTION");
    }

    lastIsConnectedState = currentConnectivity;
  }

  // Update the map
  map->Update((float)elapsed);

  // Update the camera
  camera.Update((float)elapsed);
  
  // Loop the bg
  bg->Update((float)elapsed);

  // Draw navi moving
  naviAnimator.Update((float)elapsed, owNavi.getSprite());

  // Move the navi down
  owNavi.setPosition(owNavi.getPosition() + sf::Vector2f(50.0f*(float)elapsed, 0));

  // TODO: fix this broken camera system! I have no idea why these values are required to look right...
  sf::Vector2f camOffset = camera.GetView().getSize();
  camOffset.x /= 5; // what?
  camOffset.y /= 3.5; // huh?

  // Follow the navi
  camera.PlaceCamera(map->ScreenToWorld(owNavi.getPosition() - sf::Vector2f(0.5, 0.5)) + camOffset);

  if (!gotoNextScene) {
    if (INPUT.Has(EventTypes::PRESSED_CONFIRM) && !INPUT.Has(EventTypes::PRESSED_CANCEL)) {

      // Folder Select
      if (menuSelectionIndex == 0) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>, swoosh::intent::milli<500>>;
        getController().push<segue::to<FolderScene>>(data);
      }

      // Config Select on PC 
      if (menuSelectionIndex == 1) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<DiamondTileSwipe<direction::right>, swoosh::intent::milli<500>>;
        getController().push<segue::to<ConfigScene>>();
      }

      // Navi select
      if (menuSelectionIndex == 2) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<250>>;
        using intent = segue::to<SelectNaviScene>;
        getController().push<intent>(currentNavi);
      }

      // Mob select
      if (menuSelectionIndex == 3) {
        gotoNextScene = true;

        CardFolder* folder = nullptr;

        if (data.GetFolder(0, folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
          getController().push<segue>(currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR); 
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
          gotoNextScene = false;
        }
      }
    }

    if (INPUT.Has(EventTypes::PRESSED_UI_UP)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to previous selection 
        selectInputCooldown = maxSelectInputCooldown;
        menuSelectionIndex--;
      }
    }
    else if (INPUT.Has(EventTypes::PRESSED_UI_DOWN)) {
      selectInputCooldown -= elapsed;

      if (selectInputCooldown <= 0) {
        // Go to next selection 
        selectInputCooldown = maxSelectInputCooldown;
        menuSelectionIndex++;
      }
    }
    else {
      selectInputCooldown = 0;
    }
  }

  // Allow player to resync with remote account by pressing the pause action
  if (INPUT.Has(EventTypes::PRESSED_PAUSE)) {
      accountCommandResponse = WEBCLIENT.SendFetchAccountCommand();
  }

  // Keep menu selection in range
  menuSelectionIndex = std::max(0, menuSelectionIndex);
  menuSelectionIndex = std::min(3, menuSelectionIndex);

  if (menuSelectionIndex != lastMenuSelectionIndex) {
    AUDIO.Play(AudioType::CHIP_SELECT);
  }

  webAccountAnimator.Update((float)elapsed, webAccountIcon.getSprite());

  lastMenuSelectionIndex = menuSelectionIndex;
}

void MainMenuScene::onLeave() {
    #ifdef __ANDROID__
    ShutdownTouchControls();
    #endif
}

void MainMenuScene::onExit()
{

}

void MainMenuScene::onEnter()
{
  // If coming back from navi select, the navi has changed, update it
  auto owPath = NAVIS.At(currentNavi).GetOverworldAnimationPath();

  if (owPath.size()) {
      owNavi.setTexture(NAVIS.At(currentNavi).GetOverworldTexture());
      naviAnimator = Animation(NAVIS.At(currentNavi).GetOverworldAnimationPath());
      naviAnimator.Reload();
      naviAnimator.SetAnimation("PLAYER_OW_RD");
      naviAnimator << Animator::Mode::Loop;
  }
  else {
      Logger::Logf("Overworld animation not found for navi at index %i", currentNavi);
  }
}

void MainMenuScene::onResume() {
  gotoNextScene = false;

  ENGINE.SetCamera(camera);

#ifdef __ANDROID__
  StartupTouchControls();
#endif
}

void MainMenuScene::onDraw(sf::RenderTexture& surface) {
  ENGINE.SetRenderSurface(surface);

  ENGINE.Draw(bg);
  ENGINE.Draw(map);

  ENGINE.Draw(overlay);

  if (showHUD) {
    uiAnimator.SetAnimation("CHIP_FOLDER");

    if (menuSelectionIndex == 0) {
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(50.f, 50.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(100.f, 50.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(20.f, 50.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(100.f, 50.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("CONFIG");

    if (menuSelectionIndex == 1) {
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(50.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CONFIG_LABEL");
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(100.f, 120.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(20.f, 120.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("CONFIG_LABEL");
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(100.f, 120.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("NAVI");

    if (menuSelectionIndex == 2) {
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(50.f, 190.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("NAVI_LABEL");
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(100.f, 190.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(20.f, 190.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("NAVI_LABEL");
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(100.f, 190.f);
      ENGINE.Draw(ui);
    }

    uiAnimator.SetAnimation("MOB_SELECT");

    if (menuSelectionIndex == 3) {
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(50.f, 260.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("MOB_SELECT_LABEL");
      uiAnimator.SetFrame(2, ui.getSprite());
      ui.setPosition(100.f, 260.f);
      ENGINE.Draw(ui);
    }
    else {
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(20.f, 260.f);
      ENGINE.Draw(ui);

      uiAnimator.SetAnimation("MOB_SELECT_LABEL");
      uiAnimator.SetFrame(1, ui.getSprite());
      ui.setPosition(100.f, 260.f);
      ENGINE.Draw(ui);
    }

    ENGINE.Draw(ui);
  }

  // Add the web account connection symbol
  ENGINE.Draw(&webAccountIcon);

}

void MainMenuScene::onEnd() {
  AUDIO.StopStream();
  ENGINE.RevokeShader();

#ifdef __ANDROID__
  ShutdownTouchControls();
#endif
}

#ifdef __ANDROID__
void MainMenuScene::StartupTouchControls() {
    ui.setScale(2.f,2.f);

    uiAnimator.SetAnimation("CHIP_FOLDER_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 50.f);

    auto bounds = ui.getGlobalBounds();
    auto rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& folderBtn = TouchArea::create(rect);

    folderBtn.onRelease([this](sf::Vector2i delta) {
        Logger::Log("folder released");

        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::left>, swoosh::intent::milli<500>>;
        getController().push<segue::to<FolderScene>>(data);
    });

    folderBtn.onTouch([this]() {
        menuSelectionIndex = 0;
    });

    uiAnimator.SetAnimation("LIBRARY_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 120.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));

    Logger::Log(std::string("rect: ") + std::to_string(rect.left) + ", " + std::to_string(rect.top) + ", " + std::to_string(rect.width) + ", " + std::to_string(rect.height));

    auto& libraryBtn = TouchArea::create(rect);

    libraryBtn.onRelease([this](sf::Vector2i delta) {
        Logger::Log("library released");

        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);

        using swoosh::intent::direction;
        using segue = swoosh::intent::segue<PushIn<direction::right>>;
        getController().push<segue::to<LibraryScene>, swoosh::intent::milli<500>>();
    });

    libraryBtn.onTouch([this]() {
        menuSelectionIndex = 1;
    });


    uiAnimator.SetAnimation("NAVI_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 190.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& naviBtn = TouchArea::create(rect);

    naviBtn.onRelease([this](sf::Vector2i delta) {
        gotoNextScene = true;
        AUDIO.Play(AudioType::CHIP_DESC);
        using segue = swoosh::intent::segue<Checkerboard, swoosh::intent::milli<500>>;
        using intent = segue::to<SelectNaviScene>;
        getController().push<intent>(currentNavi);
    });

    naviBtn.onTouch([this]() {
        menuSelectionIndex = 2;
    });

    uiAnimator.SetAnimation("MOB_SELECT_LABEL");
    uiAnimator.SetFrame(1, ui);
    ui.setPosition(100.f, 260.f);

    bounds = ui.getGlobalBounds();
    rect = sf::IntRect(int(bounds.left), int(bounds.top), int(bounds.width), int(bounds.height));
    auto& mobBtn = TouchArea::create(rect);

    mobBtn.onRelease([this](sf::Vector2i delta) {
        gotoNextScene = true;

        CardFolder* folder = nullptr;

        if (data.GetFolder("Default", folder)) {
          AUDIO.Play(AudioType::CHIP_DESC);
          using segue = swoosh::intent::segue<PixelateBlackWashFade, swoosh::intent::milli<500>>::to<SelectMobScene>;
          getController().push<segue>(currentNavi, *folder);
        }
        else {
          AUDIO.Play(AudioType::CHIP_ERROR);
          Logger::Log("Cannot proceed to mob select. Error selecting folder 'Default'.");
          gotoNextScene = false;
        }
    });

    mobBtn.onTouch([this]() {
        menuSelectionIndex = 3;
    });
}

void MainMenuScene::ShutdownTouchControls() {
  TouchArea::free();
}
#endif