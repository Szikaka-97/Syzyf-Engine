# Dokumentacja Silnika Syzyf

## Wprowadzenie

Syzyf to prosty silnik gry napisany w języku C++ z wykorzystaniem OpenGL, GLFW oraz bibliotek takich jak GLM, Assimp, stb_image, FreeType i innych. Silnik jest zaprojektowany do tworzenia prostych aplikacji graficznych i gier 3D.

## Architektura Silnika

Silnik opiera się na komponentowej architekturze z systemami zarządzającymi różnymi aspektami aplikacji:

- **Engine**: Główna klasa silnika odpowiedzialna za inicjalizację okna, kontekstu OpenGL i głównej pętli aplikacji
- **Scene**: Zarządzanie scenami i hierarchią obiektów
- **GameObject**: Obiekty gry składające się z komponentów
- **Systemy**: Specjalizowane systemy obsługujące różne funkcjonalności (grafika, wejście, czas, oświetlenie, itp.)

## Podstawowe Funkcjonalności

### 1. Inicjalizacja i Główna Pętla

Silnik inicjalizuje się poprzez wywołanie `Engine::Init()` z parametrami okna, a następnie uruchamia główną pętlę poprzez `Engine::Run()`. Główna pętla obsługuje:
- Aktualizację czasu
- Przetwarzanie wejścia
- Aktualizację sceny
- Renderowanie
- Obsługę zdarzeń GUI (ImGui)

```cpp
// Przykład inicjalizacji
Engine::Init(1280, 720, "Moja Gra");
Engine::Run();
```

### 2. Zarządzanie Scenami

Sceny zawierają hierarchię GameObject'ów i zarządzają ich cyklem życia.

**Klasa Scene:**
- Zawiera kolekcję GameObject'ów
- Obsługuje aktualizację wszystkich obiektów
- Zarządza komponentami

### 3. Obiekty Gry (GameObject)

GameObject'y to podstawowe jednostki w silniku, składające się z komponentów.

**Podstawowe komponenty:**
- **Transform**: Pozycja, rotacja, skala w przestrzeni 3D
- **MeshRenderer**: Renderowanie siatek 3D z materiałami
- **Camera**: Kamery do renderowania sceny
- **Light**: Źródła światła

```cpp
// Przykład tworzenia GameObject'a
GameObject* obj = new GameObject("MojObiekt");
obj->AddComponent<Transform>();
obj->AddComponent<MeshRenderer>();
```

### 4. System Graficzny

System graficzny obsługuje renderowanie 3D z wykorzystaniem OpenGL 4.6.

**Kluczowe klasy:**
- **Graphics**: Główny system renderowania
- **Shader**: Zarządzanie shaderami GLSL
- **Material**: Materiały z parametrami renderowania
- **Texture**: Ładowanie i zarządzanie teksturami
- **Mesh**: Reprezentacja siatek 3D

**Wspierane efekty:**
- Bloom
- Tone mapping
- Post-processing
- Skybox
- Reflection probes

### 5. System Wejścia

InputSystem obsługuje wejście z klawiatury, myszki i innych urządzeń.

**Możliwości:**
- Sprawdzanie stanu klawiszy
- Pozycja i ruch myszki
- Obsługa zdarzeń

```cpp
// Przykład sprawdzania wejścia
if (InputSystem::GetKey(KeyCode::W)) {
    // Ruch do przodu
}
```

### 6. System Czasu

TimeSystem zarządza czasem w aplikacji.

**Funkcjonalności:**
- Delta time między klatkami
- Czas całkowity
- Kontrola czasu (pause/resume)

```cpp
// Przykład użycia
float deltaTime = TimeSystem::GetDeltaTime();
float totalTime = TimeSystem::GetTime();
```

### 7. Zarządzanie Zasobami

Resources zarządza ładowaniem i cachowaniem zasobów.

**Obsługiwane zasoby:**
- Modele 3D (przez Assimp)
- Tekstury
- Shadery
- Czcionki

**Przykład ładowania zasobów:**

```cpp
// Ładowanie tekstury
Texture2D* texture = Resources::Load<Texture2D>("textures/wall.png");

// Ładowanie modelu 3D
Mesh* mesh = Resources::Load<Mesh>("models/cube.obj");

// Ładowanie shadera
ShaderProgram* shader = Resources::Load<ShaderProgram>("shaders/basic");

// Ładowanie czcionki
Font* font = Resources::Load<Font>("fonts/arial.ttf");

// Użycie w materiale
Material* material = new Material(shader);
material->SetTexture("diffuseTexture", texture);

// Użycie w rendererze
MeshRenderer* renderer = obj->AddComponent<MeshRenderer>();
renderer->SetMesh(mesh);
renderer->SetMaterial(material);
```

**System cachowania zasobów:**

Silnik automatycznie cache'uje załadowane zasoby. Wielokrotne wywołania `Resources::Load()` dla tego samego zasobu zwrócą tę samą instancję, co oszczędza pamięć i czas ładowania.

```cpp
// Pierwszego wywołania ładuje zasób
Texture2D* tex1 = Resources::Load<Texture2D>("textures/wall.png");

// Drugie wywołanie zwraca tę samą instancję z cache'a
Texture2D* tex2 = Resources::Load<Texture2D>("textures/wall.png");

assert(tex1 == tex2); // true
```

### 8. Tworzenie Shaderów

Silnik używa GLSL shaderów zorganizowanych w programy. Shadery znajdują się w katalogu `res/shaders/`.

**Struktura shaderów:**
```
res/shaders/
├── basic/           # Katalog shadera
│   ├── vertex.glsl  # Vertex shader
│   ├── fragment.glsl # Fragment shader
│   └── uniforms.json # Specyfikacja uniformów (opcjonalne)
```

**Przykład prostego vertex shadera (basic/vertex.glsl):**
```glsl
#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vNormal;
out vec2 vTexCoord;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vNormal = aNormal;
    vTexCoord = aTexCoord;
}
```

**Przykład prostego fragment shadera (basic/fragment.glsl):**
```glsl
#version 460 core

in vec3 vNormal;
in vec2 vTexCoord;

uniform sampler2D uDiffuseTexture;
uniform vec3 uLightColor;
uniform float uAmbientStrength;

out vec4 FragColor;

void main() {
    vec3 ambient = uAmbientStrength * uLightColor;
    vec4 texColor = texture(uDiffuseTexture, vTexCoord);
    FragColor = vec4(ambient * texColor.rgb, texColor.a);
}
```

**Specyfikacja uniformów (uniforms.json):**
```json
{
    "uniforms": [
        {
            "name": "uModel",
            "type": "mat4"
        },
        {
            "name": "uView", 
            "type": "mat4"
        },
        {
            "name": "uProjection",
            "type": "mat4"
        },
        {
            "name": "uDiffuseTexture",
            "type": "sampler2D"
        },
        {
            "name": "uLightColor",
            "type": "vec3"
        },
        {
            "name": "uAmbientStrength",
            "type": "float"
        }
    ]
}
```

**Tworzenie i używanie shaderów w kodzie:**

```cpp
// Ładowanie shadera
ShaderProgram* shader = Resources::Load<ShaderProgram>("shaders/basic");

// Tworzenie materiału z shaderem
Material* material = new Material(shader);

// Ustawianie wartości uniformów
material->SetMatrix4("uModel", modelMatrix);
material->SetMatrix4("uView", viewMatrix);
material->SetMatrix4("uProjection", projectionMatrix);
material->SetTexture("uDiffuseTexture", diffuseTexture);
material->SetVector3("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
material->SetFloat("uAmbientStrength", 0.1f);

// Użycie w MeshRenderer
MeshRenderer* renderer = obj->AddComponent<MeshRenderer>();
renderer->SetMaterial(material);
```

**Typy uniformów obsługiwane przez silnik:**
- `float`, `vec2`, `vec3`, `vec4`
- `int`, `ivec2`, `ivec3`, `ivec4`
- `uint`, `uvec2`, `uvec3`, `uvec4`
- `mat2`, `mat3`, `mat4`
- `sampler2D`, `samplerCube`
- Buffery uniform (Uniform Buffer Objects)

**Compute shadery:**

Silnik obsługuje również compute shadery dla obliczeń GPU:

```glsl
// compute.glsl
#version 460 core

layout(local_size_x = 32, local_size_y = 32) in;

layout(rgba32f, binding = 0) uniform image2D outputImage;

void main() {
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    vec4 color = vec4(pixel.x / 1024.0, pixel.y / 768.0, 0.5, 1.0);
    imageStore(outputImage, pixel, color);
}
```

```cpp
// Użycie compute shadera
ComputeShaderProgram* computeShader = Resources::Load<ComputeShaderProgram>("shaders/compute");
computeShader->Dispatch(1024/32, 768/32, 1);
```

### 8. System Oświetlenia

LightSystem obsługuje różne typy źródeł światła.

**Typy światła:**
- Directional (kierunkowe)
- Point (punktowe)
- Spot (reflektorowe)

### 9. Kamera

Camera obsługuje perspektywy i rzutowanie.

**Tryby kamery:**
- Perspektywa
- Ortogonalna
- Frustum culling

### 10. Transformacje

Transform zarządza pozycją, rotacją i skalą obiektów w przestrzeni 3D.

**Możliwości:**
- Hierarchiczne transformacje
- Macierze transformacji
- Bounding box'y

## API i Użycie

### Inicjalizacja Silnika

```cpp
#include <Engine.h>

int main() {
    Engine::Init(1280, 720, "Tytuł Gry");
    // Ustawienie sceny głównej
    Engine::SetRootScene(new Scene("MainScene"));
    Engine::Run();
    return 0;
}
```

### Tworzenie Sceny

```cpp
Scene* scene = new Scene("MojaScena");

// Dodanie obiektu do sceny
GameObject* obj = new GameObject("Obiekt");
scene->AddGameObject(obj);

// Ustawienie jako scena główna
Engine::SetRootScene(scene);
```

### Dodawanie Komponentów

```cpp
GameObject* obj = new GameObject("Obiekt3D");

// Dodanie transformacji
Transform* transform = obj->AddComponent<Transform>();
transform->SetPosition(glm::vec3(0, 0, 0));

// Dodanie renderera siatki
MeshRenderer* renderer = obj->AddComponent<MeshRenderer>();
renderer->SetMesh(Resources::LoadMesh("model.obj"));
renderer->SetMaterial(Resources::LoadMaterial("material.mat"));
```

### Renderowanie

System renderowania automatycznie obsługuje wszystkie MeshRenderer'y w scenie. Kamery renderują scenę do buforów ramki z opcjami post-processingu.

### Wejście

```cpp
void Update() {
    if (InputSystem::GetKeyDown(KeyCode::SPACE)) {
        // Akcja po naciśnięciu spacji
    }
    
    glm::vec2 mousePos = InputSystem::GetMousePosition();
    glm::vec2 mouseDelta = InputSystem::GetMouseDelta();
}
```

## Zależności

Silnik wykorzystuje następujące biblioteki:
- GLFW (okna i wejście)
- GLAD (ładowanie OpenGL)
- GLM (matematyka)
- Assimp (ładowanie modeli)
- stb_image (ładowanie tekstur)
- FreeType (czcionki)
- ImGui (interfejs użytkownika)
- spdlog (logowanie)

## Budowanie Projektu

Projekt używa CMake do budowania. Szczegóły w README.md.

## Rozszerzalność

Silnik jest zaprojektowany do rozszerzania poprzez:
- Dodawanie nowych komponentów
- Implementację nowych systemów
- Rozszerzanie funkcjonalności renderowania
- Dodawanie nowych typów zasobów

## Ograniczenia

- Obecnie obsługuje tylko OpenGL
- Brak wsparcia dla dźwięku
- Ograniczona obsługa formatów plików
- Brak wbudowanego edytora

## Przyszłe Funkcjonalności

Na podstawie TODO.md, planowane są:
- Zaawansowane systemy cieniowania
- Fizyka
- Dźwięk
- Sieć
- Więcej efektów post-processingu
- Optymalizacje wydajności