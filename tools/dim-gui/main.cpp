#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL_opengles2.h>
#else
#include <SDL_opengl.h>
#endif

#include "ImGuiFileDialog.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <cstdint>

#include "IconsFontAwesome5.h"
#include "IconsFontAwesome5Brands.h"
// extern unsigned int IconsFontAwesome5_compressed_data[119716/4];

static std::string log_read(const std::string &filename)
{
  std::ifstream file;
  file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  std::stringstream file_stream;
  try
  {
    file.open(filename.c_str(), std::ios::binary);
    // std::cout << ' ' << std:setw(2) << (char) file.read()
    char c = file.get();

    while (file.good())
    {
      std::cout << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)c);
      std::cout << "";
      file_stream << std::setfill('0') << std::setw(2) << std::hex << (0xff & (unsigned int)c);
      file_stream << " ";
      std::cout << " ";
      c = file.get();
    }
    std::cout << std::endl;

    // file_stream << file.rdbuf();
    file.close();
  }
  catch (std::ifstream::failure e)
  {
    std::cout << "Error reading Log File!" << std::endl;
  }
  return file_stream.str();
}

int main(int, char **)
{
  // Setup SDL
  // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
  // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
  {
    printf("Error: %s\n", SDL_GetError());
    return -1;
  }

  // Decide GL+GLSL versions
#if defined(IMGUI_IMPL_OPENGL_ES2)
  // GL ES 2.0 + GLSL 100
  const char *glsl_version = "#version 100";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
  // GL 3.2 Core + GLSL 150
  const char *glsl_version = "#version 150";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
  // GL 3.0 + GLSL 130
  const char *glsl_version = "#version 130";
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

  // Create window with graphics context
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  SDL_Window *window = SDL_CreateWindow("DIM", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, window_flags);
  SDL_GLContext gl_context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, gl_context);
  SDL_GL_SetSwapInterval(1); // Enable vsync

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(io.DisplaySize);

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();

// Docking
// ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

// single window
#ifdef IMGUI_HAS_VIEWPORT
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->GetWorkPos());
  ImGui::SetNextWindowSize(viewport->GetWorkSize());
  ImGui::SetNextWindowViewport(viewport->ID);
#else
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
  ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

  ImGui::GetIO().FontGlobalScale = 1.0 / 0.5;

  // Font
  ImGui::GetIO().Fonts->AddFontDefault();
  ImFontConfig icons_config;
  icons_config.MergeMode = true;
  icons_config.PixelSnapH = true;
  const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
  ImGui::GetIO().Fonts->AddFontFromFileTTF("fonts/fontawesome-webfont.ttf", 12.0f, &icons_config, icons_ranges);
  // use FONT_ICON_FILE_NAME_FAR if you want regular instead of solid

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init(glsl_version);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  std::string dim_info;
  std::string log_str;
  std::string log_plaintext;
  std::string log_gps;

  // Main loop
  bool done = false;
  while (!done)
  {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
      ImGui_ImplSDL2_ProcessEvent(&event);
      if (event.type == SDL_QUIT)
        done = true;
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
        done = true;
    }

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    // render your GUI
    // ImGui::Begin("ETRI DIM Log Analysis");
    // render your GUI for single window
    ImGui::Begin("ETRI DIM Log Analysis", 0, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

    if (ImGui::Button(ICON_FA_USB " Connect DIM"))
    {
      std::cout << "Connected\n";

      dim_info = " DIM Version 2.00.E5, Serial Number: 2020240000000048";
    }

    ImGui::SameLine();

    // open Dialog Simple
    if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open"))
      ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".txt,.cpp", ".");

    // display
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
    {
      // action if OK
      if (ImGuiFileDialog::Instance()->IsOk())
      {
        std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
        std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
        // action
        std::cout << filePathName << std::endl;
        log_str = log_read(filePathName);
        std::cout << log_str;
        std::cout << '----';
      }

      // close
      ImGuiFileDialog::Instance()->Close();
    }

    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_KEY " Decryption"))
    {
      log_plaintext = "FE 1C 96 00 00 21 C2 C6 03 00 8 E A E 5 8 1 7 B 2 4 D 8 7 4 B 3 E 8 D 00 00 72 FF FF FF";
      log_gps = " Lat: 38.23094, Lon: 127.32023, Alt: 12.3";
      std::cout << "Decryption\n";
    }
    ImGui::SameLine();
    if (ImGui::Button(ICON_FA_KEY " Encryption"))
    {
      std::cout << "Encryption\n";
    }

    ImGui::Separator();

    static float w = 600.0f;
    static float h = 300.0f;

    ImGui::TextColored(ImVec4(0, 1, 0, 0.5), " MAVLink #33 GPS:");
    ImGui::SameLine();
    ImGui::Text(log_gps.c_str());

    ImGui::Separator();

    ImGui::TextColored(ImVec4(1, 1, 0, 1), dim_info.c_str());
    ImGui::TextColored(ImVec4(1, 1, 0, 1), " Ciphertext                                  Plaintext                                  MAVLink");
    ImGui::BeginChild("ENCRYPTED", ImVec2(w, 0), true);
    // for (int n = 0; n < 10; n++)
    //   ImGui::Text("%04d: Some text", n);
    ImGui::TextWrapped(log_str.c_str());
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("DECRYPT", ImVec2(w, 0), true);
    ImGui::TextWrapped(log_plaintext.c_str());
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("MAVLink Protocol", ImVec2(w, 0), true);
    ImGui::TextWrapped(log_gps.c_str());
    ImGui::EndChild();

    ImGui::End();
    // ImGui::PopStyleVar(2);

    // Rendering
    ImGui::Render();
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }

  // Cleanup
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
