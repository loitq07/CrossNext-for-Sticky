#pragma once

// The deck: 110 cards of five words, in the order of the owner's list
// (src/games/oneclue/README.md has the spelling fixes and the two added words).
// Capitals, because every label on a Toybox screen is.
namespace oneclue {

constexpr int kCards = 110;
constexpr int kWords = 5;

constexpr const char* CARDS[kCards][kWords] = {
    {"TANGO", "RIPE", "TEMPLE", "SAND", "FRIES"},  // 1
    {"CRANE", "OPERATION", "RAP", "MUSE", "DEFENSE"},  // 2
    {"OPRAH", "NINJA", "COMFORTER", "HUNTER", "VEGETABLE"},  // 3
    {"KNIFE", "PILLOW", "ARMSTRONG", "SWORD", "FLIGHT"},  // 4
    {"CEMETERY", "CUPID", "UMBRELLA", "LEAP", "ROOT"},  // 5
    {"SHIP", "BOWLING", "CHURCHILL", "RAM", "SPY"},  // 6
    {"HAMMER", "CHILE", "GUMBO", "EMPEROR", "POPE"},  // 7
    {"FOX", "POLICE", "SIMPSON", "LIGHTNING", "NEWSPAPER"},  // 8
    {"TOWEL", "VENUS", "OCTOPUS", "CYCLE", "OPERA"},  // 9
    {"CANVAS", "VAMPIRE", "MONOPOLY", "CARTOON", "HOTEL"},  // 10
    {"BARBECUE", "PANDA", "DREAM", "MARRIAGE", "BELLYBUTTON"},  // 11
    {"DWARF", "CIRCUS", "ELVIS", "MOWER", "STONE"},  // 12
    {"GODZILLA", "UNIFORM", "RAIN", "FIRE", "HELMET"},  // 13
    {"CATERPILLAR", "HUMOR", "CORNER", "ANTARCTICA", "SAUSAGE"},  // 14
    {"IRON", "BATMAN", "SONG", "NILE", "CINEMA"},  // 15
    {"SOMBRERO", "CLOVER", "BUTTON", "CHEESE", "CHEWBACCA"},  // 16
    {"BAKER", "PERFUME", "FLAME", "ZOMBIE", "JOKER"},  // 17
    {"JACKSON", "CIGARETTE", "BRACELET", "WEATHER", "TOWER"},  // 18
    {"LIGHT", "SOFA", "MOZART", "RING", "PIZZA"},  // 19
    {"BRAIN", "MASK", "CONCERT", "TROY", "SHARK"},  // 20
    {"MELON", "ANCHOR", "ISRAEL", "CACTUS", "TENNIS"},  // 21
    {"KING", "BALLET", "BAND", "ALCOHOL", "LAVA"},  // 22
    {"RADIO", "PEAR", "GLADIATOR", "SUN", "CEREAL"},  // 23
    {"MUMMY", "BATTERY", "FAIR", "KARATE", "PARROT"},  // 24
    {"ALCATRAZ", "SMOKE", "HAZELNUT", "DIAMOND", "ROSE"},  // 25
    {"HOLLYWOOD", "MOSQUITO", "SPEAR", "PURSE", "END"},  // 26
    {"SPARTACUS", "FUR", "TORNADO", "PYRAMID", "KANGAROO"},  // 27
    {"TREASURE", "PILOT", "MICKEY", "SEWER", "GALAXY"},  // 28
    {"STRING", "AUSTRALIA", "CASTLE", "GUARD", "SHEEP"},  // 29
    {"CROISSANT", "GHOST", "STRAW", "NAIL", "POTTER"},  // 30
    {"LONELY", "POTATO", "WAVE", "SCHOOL", "LEGO"},  // 31
    {"FAILURE", "HISTORY", "BEER", "DISCO", "PRESIDENT"},  // 32
    {"ICE", "FLUTE", "CHAMPAGNE", "SAFARI", "ALIEN"},  // 33
    {"CAT", "PALACE", "ELECTION", "HONEY", "RAMBO"},  // 34
    {"SAIL", "CHEDDAR", "SHREK", "POLE", "WESTERN"},  // 35
    {"MIRROR", "PROM", "BATH", "PIT", "FAIRY"},  // 36
    {"JEWELRY", "SUBWAY", "GLASSES", "STING", "SATURN"},  // 37
    {"CANE", "MUSKETEER", "THREAD", "TULIP", "IKEA"},  // 38
    {"OLYMPICS", "CLIMB", "LAWYER", "TOLKIEN", "RIVER"},  // 39
    {"TRAIN", "SHRIMP", "ROOM", "CLEOPATRA", "WINDOW"},  // 40
    {"HEART", "MOSCOW", "POOL", "UNICORN", "ORANGE"},  // 41
    {"WIDOWMAKER", "DOCTOR", "STRAWBERRY", "NUCLEAR", "LOTTERY"},  // 42
    {"GOOGLE", "BOW", "MARS", "GOLF", "TICKET"},  // 43
    {"OVEN", "SOCKET", "EASTER", "HOSE", "RAIL"},  // 44
    {"GOAT", "BREAD", "DEVIL", "PRIMARY", "TARZAN"},  // 45
    {"GUILLOTINE", "MAGICIAN", "HOCKEY", "BANANA", "FITZGERALD"},  // 46
    {"BARBIE", "CHOCOLATE", "SNOW", "TIE", "WIND"},  // 47
    {"COCKTAIL", "MARIO", "CORK", "VIOLIN", "PEACH"},  // 48
    {"PIRATE", "VACATION", "ELF", "MAGNET", "FORK"},  // 49
    {"PUMP", "ALADDIN", "TUBE", "BELT", "BAR"},  // 50
    {"RAT", "PLIER", "AMAZON", "TOBACCO", "RULER"},  // 51
    {"SHELL", "SHAKESPEARE", "GIANT", "FOAM", "CAVE"},  // 52
    {"REGISTER", "PLAYSTATION", "BLOND", "IRIS", "LIMB"},  // 53
    {"MOUNTAIN", "PEANUT", "LIGHTBULB", "JEDI", "PIMENTO"},  // 54
    {"BOARD", "FIREMAN", "GLASSES", "BURRITO", "GREECE"},  // 55
    {"TATTOO", "SPIELBERG", "APPLE", "SIREN", "BOXING"},  // 56
    {"MOUTH", "CAROUSEL", "PSYCHO", "GRASS", "FALL"},  // 57
    {"DARWIN", "TOMATO", "PARACHUTE", "CANNON", "BINOCULARS"},  // 58
    {"KNIGHT", "PEACE", "FLOWER", "SWITZERLAND", "CALENDAR"},  // 59
    {"HALLOWEEN", "CHIP", "BABY", "CANTEEN", "PAIR"},  // 60
    {"MIRAGE", "RAMSES", "BONFIRE", "CROSSROADS", "PRINCESS"},  // 61
    {"MAFIA", "LARGE", "MISSILE", "MICROSOFT", "SKI"},  // 62
    {"REGGAE", "MANURE", "LAKE", "MONKEY", "LIGHTHOUSE"},  // 63
    {"SCREW", "RAKE", "COMPUTER", "STARBUCKS", "BALL"},  // 64
    {"POISON", "STAR", "WOLF", "JONES", "ANNIVERSARY"},  // 65
    {"DOPING", "GARLIC", "CUBE", "ROCKY", "MILK"},  // 66
    {"HOUSE", "GODFATHER", "WAR", "ROPE", "WINE"},  // 67
    {"PEPPER", "TRIANGLE", "DOLL", "ITALY", "SCENE"},  // 68
    {"GROTTO", "FORD", "CASINO", "CANDY", "FOUNTAIN"},  // 69
    {"CLUB", "CHRISTMAS", "FASHION", "STATION", "LAMP"},  // 70
    {"THUNDER", "GARDEN", "PAINTING", "SHACK", "TRUCE"},  // 71
    {"NUMBER", "DECATHLON", "RUN", "METAL", "TUNA"},  // 72
    {"VIKING", "ALARM", "DANCE", "HULK", "DESERT"},  // 73
    {"HANUKKAH", "ELASTIC", "PAN", "FLASH", "TUNNEL"},  // 74
    {"LADDER", "ANGEL", "MAD", "HAIR", "MATRIX"},  // 75
    {"PUPPET", "GAME", "VEGAS", "SAFE", "PLANE"},  // 76
    {"FRANCE", "MOON", "CAFETERIA", "HANDLE", "TOOL"},  // 77
    {"EXPLOSION", "PENGUIN", "CELL", "GANDHI", "OASIS"},  // 78
    {"BUTTERFLY", "POWDER", "PORCELAIN", "MARKET", "COCOON"},  // 79
    {"NEIGHBORHOOD", "ROCKY", "TIGER", "NEEDLE", "SOAP"},  // 80
    {"STEW", "CANADA", "ZOO", "PIPE", "BOOK"},  // 81
    {"SUGAR", "THEATER", "SHOVEL", "DUNE", "PREGNANT"},  // 82
    {"PLASTIC", "RAY", "CARTON", "PEBBLE", "EVEREST"},  // 83
    {"CARPET", "PONY", "CROWN", "NEW", "TARANTINO"},  // 84
    {"TERMINATOR", "LETTER", "DRAG", "PARADISE", "EGG"},  // 85
    {"SYRUP", "FOREST", "SCALE", "ZEUS", "COCKROACH"},  // 86
    {"MUSIC", "THROAT", "AMERICA", "COFFEE", "FEVER"},  // 87
    {"BUFFY", "VOLCANO", "PASSION", "ROOSTER", "ELECTRICITY"},  // 88
    {"THOUGHT", "FRANKENSTEIN", "SHELF", "ACCENT", "SHOWER"},  // 89
    {"TRADITION", "SNAKE", "CUP", "PICASSO", "WATCH"},  // 90
    {"BRANCH", "CINDERELLA", "CREPE", "ISLAND", "TAXI"},  // 91
    {"AVATAR", "MILL", "JUNGLE", "NUN", "FIRECRACKER"},  // 92
    {"DINOSAUR", "YELLOW", "MUSHROOM", "PIGEON", "PIKACHU"},  // 93
    {"PRISON", "HOLE", "PUNK", "EVENING", "MAP"},  // 94
    {"VENGEANCE", "BOSS", "EMERGENCY", "CROSSROADS", "WALTZ"},  // 95
    {"CAVITY", "SLEEVE", "GREMLINS", "POKER", "PIE"},  // 96
    {"CAESAR", "NOODLE", "HAT", "DENTIST", "WHEAT"},  // 97
    {"SERIES", "NEST", "SPICE", "ELEPHANT", "CARNIVAL"},  // 98
    {"FLINTSTONE", "ROBOT", "COMEDY", "LANGUAGE", "HAIRDRESSER"},  // 99
    {"GRENADE", "STUFFING", "BRUSH", "PIG", "HUMAN"},  // 100
    {"TOKYO", "HELL", "CHICKEN", "HELICOPTER", "COLONEL"},  // 101
    {"GENIUS", "DRACULA", "LION", "SOCK", "FRIDAY"},  // 102
    {"NINTENDO", "BET", "SALT", "MANUAL", "FROST"},  // 103
    {"MYTH", "FACEBOOK", "ACORN", "BONE", "BRIDGE"},  // 104
    {"CAKE", "STALLION", "MEXICO", "WHITE", "BALD"},  // 105
    {"BERRY", "STUDY", "GOTHIC", "TITANIC", "MACHINE"},  // 106
    {"MUSTACHE", "BUBBLE", "CHAIN", "STARK", "COOKIE"},  // 107
    {"SLIPPER", "LEAF", "COUGAR", "REVOLUTION", "SAHARA"},  // 108
    {"POLAR", "MOUSE", "NECKLACE", "FARM", "BELGIUM"},  // 109
    {"LADYBUG", "MUSTARD", "SHERLOCK", "BOTTLE", "VIRUS"},  // 110
};

}  // namespace oneclue
