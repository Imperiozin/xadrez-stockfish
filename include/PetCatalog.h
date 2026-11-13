#pragma once
// Modelos e animações Cube Pets: Kenney — https://kenney.nl (licença CC0).
#include <string>

struct PetDefinition {
    const char *id;
    const char *file;
    const char *name;
};

inline constexpr PetDefinition petCatalog[] = {
    {"Beaver", "beaver", "Castor"}, {"Bee", "bee", "Abelha"},
    {"Bunny", "bunny", "Coelho"}, {"Cat", "cat", "Gato"},
    {"Caterpillar", "caterpillar", "Lagarta"}, {"Chick", "chick", "Pintinho"},
    {"Cow", "cow", "Vaca"}, {"Crab", "crab", "Caranguejo"},
    {"Deer", "deer", "Cervo"}, {"Dog", "dog", "Cachorro"},
    {"Elephant", "elephant", "Elefante"}, {"Fish", "fish", "Peixe"},
    {"Fox", "fox", "Raposa"}, {"Giraffe", "giraffe", "Girafa"},
    {"Hog", "hog", "Javali"}, {"Koala", "koala", "Coala"},
    {"Lion", "lion", "Leão"}, {"Monkey", "monkey", "Macaco"},
    {"Panda", "panda", "Panda"}, {"Parrot", "parrot", "Papagaio"},
    {"Penguin", "penguin", "Pinguim"}, {"Pig", "pig", "Porco"},
    {"Polar", "polar", "Urso polar"}, {"Tiger", "tiger", "Tigre"}
};

inline const PetDefinition *findPet(const std::string &id) {
    for (const auto &pet : petCatalog) if (id == pet.id) return &pet;
    return nullptr;
}

inline std::string petModelPath(const PetDefinition &pet) {
    return std::string("assets/objects/pets/animal-") + pet.file + ".fbx";
}
