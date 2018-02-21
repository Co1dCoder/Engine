//
//  Author: Shervin Aflatooni
//

#include "Engine.h"
#include "Logger.h"
#include "Ray.h"
#include "GuiManager.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

#include <limits>

#ifdef EMSCRIPTEN
#include <emscripten.h>

static Engine *instance = NULL;
#endif

Engine::Engine(Game *game)
{
  log_info("Initializing SDL");
  m_window = std::make_unique<Window>();

  log_info("Initializing GLEW");
  m_glewManager = std::make_unique<GLEWManager>();

  log_info("Initializing GL");
  m_glManager = std::make_unique<GLManager>(m_window.get());

  log_info("Initializing Physics Manager");
  m_physicsManager = std::make_unique<PhysicsManager>();

  this->game = game;

  quit = false;
}

Engine::~Engine(void)
{
}

void Engine::start(void)
{
  game->setEngine(this);

  game->getRootScene()->registerWithEngineAll(this);

  log_info("Initializing game");

  m_window->init();

  game->init(m_glManager.get());

  m_window->makeCurrentContext();

  m_time = std::chrono::high_resolution_clock::now();

#ifdef EMSCRIPTEN
  instance = this;

  emscripten_set_main_loop(Engine::loop, 0, 1);
#else
  while (!quit)
  {
    tick();
  }
#endif
}

#ifdef EMSCRIPTEN
void Engine::loop(void)
{
  instance->tick();
}
#endif

void Engine::tick(void)
{
  m_lastTime = m_time;
  m_time = std::chrono::high_resolution_clock::now();
  m_deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(m_time - m_lastTime);

  m_window->tick();
  m_window->getGuiManager()->tick(m_window.get(), m_deltaTime);

  quit = m_window->shouldQuit();

  m_physicsManager->tick(m_deltaTime);

  game->update(m_window->getInput(), m_deltaTime);

  game->render(m_glManager.get());

  if (m_window->getInput()->mouseIsPressed(SDL_BUTTON_LEFT))
  {
    Ray ray = Ray::getPickRay(m_window->getInput()->getMousePosition(), m_window->getViewport(), m_glManager->getViewMatrix(), m_glManager->getProjectionMatrix());

    Entity *pickedEntity = m_physicsManager->pick(&ray);

    if (pickedEntity != nullptr)
      m_glManager->drawEntity(pickedEntity);

    m_glManager->drawLine(ray.getLine(100.0f));
  }

  static bool f1Pressed = false;

  if (!f1Pressed && m_window->getInput()->isPressed(SDLK_F1))
  {
    f1Pressed = true;
    m_window->getGuiManager()->togglePropertyEditor();
  }
  else if (f1Pressed && m_window->getInput()->isReleased(SDLK_F1))
  {
    f1Pressed = false;
  }

  m_window->getGuiManager()->render(game->getRootScene().get());

  m_window->swapBuffer();
}

Window *Engine::getWindow(void) const
{
  return m_window.get();
}

GLManager *Engine::getGLManager(void) const
{
  return m_glManager.get();
}

PhysicsManager *Engine::getPhysicsManager(void) const
{
  return m_physicsManager.get();
}

std::chrono::microseconds Engine::getDeltaTime(void) const
{
  return m_deltaTime;
}
