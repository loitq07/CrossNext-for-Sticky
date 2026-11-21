#include "HeartsScreens.h"

#include <I18n.h>

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "../cards/CardsLabels.h"
#include "../ui/CardPanels.h"
namespace heartsui {
using namespace hearts;
namespace {
fui::Rect begin(toybox::Screen& s, bool menu = false) {
  auto b = cardpanel::begin(s, tr(STR_UI_HEARTS_29FBCB), menu ? 214 : 0);
  if (menu) {
    const auto r = fui::makeRect(s.device().safeRect().right() - 214, b.y - 60, 100, 40);
    dashboard::button(s, r, tr(STR_WORDLE_MENU), ActionButton, ButtonMenu, true, nullptr, 0);
    s.target().stroke(r, dashboard::paper, 2);
  }
  return b;
}
const char* passText(Pass pass) {
  switch (pass) {
    case Pass::Left:
      return tr(STR_HEARTS_PASS_LEFT);
    case Pass::Right:
      return tr(STR_HEARTS_PASS_RIGHT);
    case Pass::Across:
      return tr(STR_HEARTS_PASS_ACROSS);
    case Pass::Hold:
      return tr(STR_HEARTS_TAP_PLAY);
  }
  return "";
}
void difficultyRow(toybox::Screen& s, fui::Rect r, const Options& o, bool setup) {
  const char* values[] = {tr(STR_HEARTS_EASY), tr(STR_REDESIGN_NORMAL), tr(STR_HEARTS_EXPERT)};
  cardpanel::setting(s, r, setup ? tr(STR_HEARTS_OPPONENTS) : tr(STR_HEARTS_DIFFICULTY), values, 3, o.difficulty,
                     ActionOption, 0);
}
void limitRow(toybox::Screen& s, fui::Rect r, const Options& o) {
  const char* values[] = {"50", "100"};
  cardpanel::setting(s, r, tr(STR_HEARTS_LIMIT), values, 2, !o.shortGame, ActionOption, 3);
}
}  // namespace
void buildMenu(toybox::Screen& s, const MenuModel& m) {
  auto b = begin(s);
  cardpanel::title(s, fui::makeRect(b.x, b.y, b.width, 44),
                   m.hasSave ? tr(STR_CARDS_IN_PROGRESS) : tr(STR_CARDS_READY));
  char label[100];
  std::snprintf(label, sizeof(label), tr(STR_HEARTS_ROUND_LOW), m.savedHand);
  dashboard::text(s, fui::makeRect(b.x, b.y + 46, b.width, 34), label, 1, fui::TextAlign::Left);
  const int top = b.y + 94, leftW = b.width * 44 / 100, gap = 12;
  auto art = fui::makeRect(b.x, top, leftW, b.bottom() - top - 94);
  dashboard::frame(s, art);
  referenceart::draw(s, art.inset({14, 14, 14, 14}), referenceart::HeartsFan);
  const char* stats[] = {tr(STR_CARDS_PLAYED), tr(STR_CARDS_WINS), tr(STR_CARDS_BEST)};
  const int values[] = {m.gamesPlayed, m.gamesWon, m.bestScore};
  cardpanel::stats(s, fui::makeRect(b.x, art.bottom() + 8, leftW, 86), stats, values);
  const char* labels[] = {m.hasSave ? tr(STR_QUEST_CONTINUE) : tr(STR_UI_START_7196E7), tr(STR_UI_NEW_GAME_F6F041),
                          tr(STR_WORDLE_HELP), tr(STR_CARDS_SETTINGS)};
  static constexpr int ids[] = {ButtonConfirm, ButtonMenu, ButtonHowTo, ButtonSettings};
  static constexpr gamepanel::Mark marks[] = {gamepanel::Mark::Play, gamepanel::Mark::New, gamepanel::Mark::Help,
                                              gamepanel::Mark::Settings};
  const int rowH = (b.bottom() - top - 24) / 4;
  for (int n = 0; n < 4; ++n)
    gamepanel::row(s, fui::makeRect(b.x + leftW + gap, top + n * (rowH + 8), b.width - leftW - gap, rowH), labels[n],
                   ActionButton, ids[n], marks[n], n == 0, nullptr, true,
                   n == 1   ? &referenceart::Page
                   : n == 3 ? &referenceart::Gear
                            : nullptr);
}
void buildSetup(toybox::Screen& s, const Options& o, bool replacing) {
  const auto b = begin(s);
  cardpanel::title(s, fui::makeRect(b.x, b.y, b.width, 48), tr(STR_UI_NEW_GAME_F6F041));
  const int row = 70;
  difficultyRow(s, fui::makeRect(b.x, b.y + 54, b.width, row), o, true);
  limitRow(s, fui::makeRect(b.x, b.y + 132, b.width, row), o);
  const auto info = fui::makeRect(b.x, b.y + 210, b.width, b.height - 300);
  dashboard::frame(s, info);
  referenceart::draw(s, fui::makeRect(info.x + 20, info.y + 8, 160, info.height - 16), referenceart::Rivals);
  guideui::text(s, fui::makeRect(info.x + 210, info.y + 8, info.width - 224, info.height - 16), tr(STR_HEARTS_RIVALS),
                3, fui::TextAlign::Left, 0);
  if (replacing)
    dashboard::text(s, fui::makeRect(b.x, b.bottom() - 82, b.width, 28), tr(STR_CARDS_REPLACE), 0,
                    fui::TextAlign::Left);
  const int w = (b.width - 12) / 2;
  dashboard::button(s, fui::makeRect(b.x, b.bottom() - 50, w, 50), tr(STR_WORDLE_CANCEL), ActionButton, ButtonCancel,
                    false, nullptr, 1);
  dashboard::button(s, fui::makeRect(b.x + w + 12, b.bottom() - 50, w, 50), tr(STR_CARDS_DEAL), ActionButton,
                    ButtonDeal, true, nullptr, 1);
}
void buildSettings(toybox::Screen& s, const Options& o, bool failed) {
  const auto b = begin(s);
  cardpanel::title(s, fui::makeRect(b.x, b.y, b.width, 48), tr(STR_CARDS_SETTINGS));
  const int h = (b.height - 158) / 4;
  difficultyRow(s, fui::makeRect(b.x, b.y + 54, b.width, h), o, false);
  limitRow(s, fui::makeRect(b.x, b.y + 62 + h, b.width, h), o);
  const char* yes[] = {tr(STR_CARDS_YES), tr(STR_CARDS_NO)};
  cardpanel::setting(s, fui::makeRect(b.x, b.y + 70 + 2 * h, b.width, h), tr(STR_CARDS_CONFIRM_NEW), yes, 2,
                     !o.confirmNew, ActionOption, 5);
  cardpanel::setting(s, fui::makeRect(b.x, b.y + 78 + 3 * h, b.width, h), tr(STR_HEARTS_HIGHLIGHT), yes, 2,
                     !o.highlight, ActionOption, 7);
  dashboard::text(s, fui::makeRect(b.x, b.bottom() - 78, b.width, 26),
                  failed ? tr(STR_CHESS_PUZZLE_SAVE_ERROR) : tr(STR_HEARTS_NEXT), 0, fui::TextAlign::Left);
  dashboard::button(s, fui::makeRect(b.x, b.bottom() - 48, b.width, 48), tr(STR_CARDS_SAVE), ActionButton, ButtonSave,
                    true, nullptr, 1);
}
namespace {
// THE TABLE IS A RAIL AND A DIAMOND (ported from CrossPlay's Hearts). Position
// means SEAT: the four plaques run down the left in play order, and a card lands
// on the table where its player sits, so "West is out of clubs" is something you
// SEE rather than something you were told.
constexpr int16_t kStatusH = 34;
constexpr int16_t kRailW = 264;
constexpr int16_t kPlaqueH = 48;
constexpr int16_t kRailGap = 10;

// A seat's plaque: who, what this hand has cost them, and where they would stand
// if the hand ended now. The seat ON TURN is inverted -- solid black is the
// loudest thing the panel has, and a plaque is small enough to afford it.
//
// A SEAT THAT HAS TAKEN NOTHING SHOWS NOTHING: the "+n" pill appears only once
// it has something to say, so the first heart of a hand is an event you see.
void seatPlaque(toybox::Screen& s, const fui::Rect& box, const SeatView& seat) {
  auto& t = s.target();
  const bool invert = seat.isTurn;
  if (invert)
    t.fill(box, dashboard::ink);
  else
    dashboard::frame(s, box, 2);
  // THREE DIGITS: a seat on 99 that takes all 26 reaches 125.
  const int16_t pad = 12, pillW = 44, totalW = 60;
  const int16_t numbersW = static_cast<int16_t>(pillW + 8 + totalW + pad);
  dashboard::text(s, fui::makeRect(box.x + pad, box.y, box.width - pad - numbersW, box.height), seat.name, 1,
                  fui::TextAlign::Left, invert);
  // The PROJECTED total (the score before this hand plus what it has put there):
  // `total` alone reads 0 for all four seats through the whole first hand.
  char running[16];
  std::snprintf(running, sizeof(running), "%d", seat.total + seat.taken);
  dashboard::text(s, fui::makeRect(box.right() - pad - totalW, box.y, totalW, box.height), running, 1,
                  fui::TextAlign::Right, invert);
  if (seat.taken > 0) {
    char points[16];
    std::snprintf(points, sizeof(points), "+%d", seat.taken);
    const auto pill = fui::makeRect(box.right() - pad - totalW - 8 - pillW, box.y + 10, pillW, box.height - 20);
    t.fill(pill, invert ? dashboard::paper : dashboard::ink, static_cast<uint8_t>(pill.height / 2));
    dashboard::text(s, pill, points, 0, fui::TextAlign::Center, !invert);
  }
}

// Where a seat's card sits, as a compass. Written once so the empty place, the
// played card and the winner's frame cannot disagree about it.
fui::Rect trickSlot(const fui::Rect& felt, Seat seat, int16_t cw, int16_t ch, int16_t gap) {
  const int16_t cx = static_cast<int16_t>(felt.x + (felt.width - cw) / 2);
  const int16_t cy = static_cast<int16_t>(felt.y + (felt.height - ch) / 2);
  switch (seat) {
    case Seat::North:
      return fui::makeRect(cx, felt.y + gap, cw, ch);
    case Seat::South:
      return fui::makeRect(cx, felt.bottom() - gap - ch, cw, ch);
    case Seat::West:
      return fui::makeRect(static_cast<int16_t>(cx - cw - gap * 2), cy, cw, ch);
    case Seat::East:
      return fui::makeRect(static_cast<int16_t>(cx + cw + gap * 2), cy, cw, ch);
  }
  return fui::makeRect(cx, cy, cw, ch);
}

// A seat's place while it is empty: card-shaped and dithered, tagged with the
// seat's name (the same word as its plaque, so it needs no key). The first
// letter alone was not enough once "BẠN" and "BẮC" both start with B. The seat
// on turn gets the heavy frame -- that is the whole of "whose go is it" here.
void drawPlace(toybox::Screen& s, const fui::Rect& rect, const char* name, bool onTurn) {
  auto& t = s.target();
  t.fill(rect, fui::Paint::dither(fui::Color::DarkGray), cardart::kRadius);
  t.stroke(rect, dashboard::ink, onTurn ? toybox::kFrame : toybox::kHairline, cardart::kRadius);
  const auto tag = fui::makeRect(rect.x + 6, rect.y + (rect.height - 30) / 2, rect.width - 12, 30);
  t.fill(tag, dashboard::paper, 8);
  t.stroke(tag, dashboard::ink, toybox::kHairline, 8);
  dashboard::text(s, tag, name, 0);
}
}  // namespace

void buildBoard(toybox::Screen& s, const BoardModel& m, Layout& l) {
  if (!m.game) return;
  const auto& g = *m.game;
  auto b = begin(s, true);
  auto& t = s.target();
  const bool passing = g.phase == Phase::Passing;
  const auto& hand = g.hands[seatIndex(Seat::South)];

  // Vertical budget, from the bottom up: the hand, a status row, the table.
  const int16_t cardH = b.height >= 380 ? cardart::kCardH : 102;
  const int16_t cardW = cardH == cardart::kCardH ? cardart::kCardW : 74;
  const int16_t handTop = static_cast<int16_t>(b.bottom() - cardH);
  const int16_t statusTop = static_cast<int16_t>(handTop - kStatusH - 4);
  const int16_t tableTop = b.y;
  const int16_t tableBottom = static_cast<int16_t>(statusTop - 4);

  // RAIL: seats down the left in play order from you.
  const int16_t railTop =
      static_cast<int16_t>(tableTop + ((tableBottom - tableTop) - (kPlaqueH * 4 + kRailGap * 3)) / 2);
  static const Seat kOrder[kSeats] = {Seat::South, Seat::West, Seat::North, Seat::East};
  for (int i = 0; i < kSeats; ++i) {
    seatPlaque(s, fui::makeRect(b.x, railTop + i * (kPlaqueH + kRailGap), kRailW, kPlaqueH),
               m.seats[seatIndex(kOrder[i])]);
  }

  const auto felt = fui::makeRect(b.x + kRailW + 16, tableTop, b.width - kRailW - 16, tableBottom - tableTop);
  l.felt = felt;
  dashboard::frame(s, felt, 2);

  if (passing) {
    // The pass owns the table: the cards you are giving away, at full size,
    // with the direction in words. Four empty places for a trick that does not
    // exist would be the largest region on screen saying nothing.
    dashboard::text(s, fui::makeRect(felt.x + 18, felt.y + 10, felt.width - 36, 34), passText(g.passDirection()), 1,
                    fui::TextAlign::Left);
    const int16_t gap = 14;
    const int16_t ch = std::min<int16_t>(cardH, static_cast<int16_t>(felt.height - 66));
    const int16_t cw = static_cast<int16_t>(ch * 3 / 4);
    uint8_t chosen[kPassCount] = {kNoCard, kNoCard, kNoCard};
    int from[kPassCount] = {-1, -1, -1};
    int n = 0;
    for (int i = 0; i < hand.count && n < kPassCount; ++i)
      if (m.picked[i]) {
        from[n] = i;
        chosen[n] = hand.at(i);
        ++n;
      }
    const int16_t sx = static_cast<int16_t>(felt.x + 18);
    const int16_t sy = static_cast<int16_t>(felt.bottom() - 14 - ch);
    for (int i = 0; i < kPassCount; ++i) {
      const auto slot = fui::makeRect(sx + i * (cw + gap), sy, cw, ch);
      if (chosen[i] == kNoCard) {
        t.fill(slot, fui::Paint::dither(fui::Color::DarkGray), cardart::kRadius);
        t.stroke(slot, dashboard::ink, toybox::kHairline, cardart::kRadius);
      } else {
        cardart::drawCardFace(s, slot, chosen[i], ch);
        // Tapping the big copy takes it back: the instinctive undo.
        dashboard::hit(s, slot, ActionHandCard, from[i]);
      }
    }
    const int16_t bx = static_cast<int16_t>(sx + kPassCount * (cw + gap));
    const int16_t bh = std::min<int16_t>(80, ch);
    const auto pass = fui::makeRect(bx, sy + ch - bh, felt.right() - bx - 14, bh);
    gamepanel::row(s, pass, tr(STR_HEARTS_PASS_CARDS), m.confirmEnabled ? ActionButton : fui::ActionId(0),
                   ButtonConfirm, gamepanel::Mark::Play, m.confirmEnabled, nullptr, false);
  } else {
    // The panel's top-left corner, which the diamond leaves empty: which hand
    // and how far through it, the one thing about its shape nothing else says.
    // trickNumber counts tricks SWEPT, so after the thirteenth a bare +1 would
    // print 14/13; the last trick stays on screen as the thirteenth.
    char label[64];
    std::snprintf(label, sizeof(label), tr(STR_HEARTS_ROUND), int(g.handNumber) + 1);
    dashboard::text(s, fui::makeRect(felt.x + 18, felt.y + 8, 170, 26), label, 0, fui::TextAlign::Left);
    const int shown = g.trickNumber >= kTricks ? int(kTricks) : int(g.trickNumber) + 1;
    std::snprintf(label, sizeof(label), I18N.getLanguage() == Language::VI ? "LƯỢT %d/%d" : "TRICK %d/%d", shown, int(kTricks));
    dashboard::text(s, fui::makeRect(felt.x + 18, felt.y + 34, 170, 26), label, 0, fui::TextAlign::Left);

    // Sized so the diamond FITS: two rows of cards plus the gaps, derived from
    // the panel so it cannot drift if the band above it moves.
    const int16_t gap = 8;
    const int16_t middle = 14;
    const int16_t ch = static_cast<int16_t>((felt.height - gap * 2 - middle) / 2);
    const int16_t cw = 80;
    for (int n = 0; n < kSeats; ++n) {
      const auto slot = trickSlot(felt, static_cast<Seat>(n), cw, ch, gap);
      l.trickCard[n] = slot;
      if (g.trick.played[n] == kNoCard)
        drawPlace(s, slot, m.seats[n].name, m.seats[n].isTurn);
      else
        cardart::drawCardFace(s, slot, g.trick.played[n], ch);
    }
    // WHICH CARD IS WINNING: the one fact you need to decide your own play. Its
    // own pass, because the frame sits outside the card and would be overdrawn
    // by whichever slot rendered after it.
    if (!g.trick.empty()) {
      const auto& won = l.trickCard[seatIndex(g.trick.winner())];
      t.stroke(fui::makeRect(won.x - 5, won.y - 5, won.width + 10, won.height + 10), dashboard::ink, toybox::kFrame,
               cardart::kRadius + 4);
    }
  }

  // The status line and its right-hand note share one row, each in its own
  // part: assembled at runtime ("WEST TAKES IT, 13 POINTS") and never wrapped.
  const int16_t half = static_cast<int16_t>(b.width * 60 / 100);
  if (m.status && *m.status)
    dashboard::text(s, fui::makeRect(b.x, statusTop, half, kStatusH), m.status, 0, fui::TextAlign::Left);
  if (m.subStatus && *m.subStatus)
    dashboard::text(s, fui::makeRect(b.x + half, statusTop, b.width - half, kStatusH), m.subStatus, 0,
                    fui::TextAlign::Right);

  // YOUR HAND. Spread as far as the cards go and no further; once it overlaps,
  // every card shows the same sliver (rank over pip) so none reads as selected.
  l.handCount = hand.count;
  if (hand.count == 0) return;
  int16_t step = static_cast<int16_t>(cardW + 10);
  if (hand.count > 1) step = std::min<int16_t>(step, static_cast<int16_t>((b.width - cardW) / (hand.count - 1)));
  const int16_t span = static_cast<int16_t>(cardW + step * (hand.count - 1));
  const int16_t x0 = static_cast<int16_t>(b.x + (b.width - span) / 2);
  const bool fanned = step < cardW;
  for (int n = 0; n < hand.count; ++n) {
    const bool picked = passing && m.picked[n];
    const bool legal = !passing && m.highlight && m.legal[n];
    const auto r = fui::makeRect(x0 + step * n, handTop, cardW, cardH);
    l.handCard[n] = r;
    cardart::drawCardFace(s, r, hand.at(n), fanned ? step : cardH, fanned ? cardart::Fan::Sideways : cardart::Fan::None,
                          picked || legal ? cardart::Ink::Picked : cardart::Ink::Normal);
    // The part of the card the player can see; hit-testing and the marks use the
    // rect that drew it rather than recomputing geometry.
    const int16_t visibleW = (fanned && n + 1 < hand.count) ? step : cardW;
    if (picked) {
      // A chosen card does not move (a lift cut through the status line and hid
      // its neighbour's index); a tab inside its own visible sliver says it.
      t.fill(fui::makeRect(r.x + 8, r.y + cardH - 32, visibleW - 16, 18), dashboard::ink, 6);
    } else if (legal) {
      t.stroke(fui::makeRect(r.x + 4, r.y + 4, visibleW - 8, r.height - 8), dashboard::ink, 2, 5);
    }
    dashboard::hit(s, fui::makeRect(r.x, r.y, visibleW, cardH), ActionHandCard, n);
  }
}
void buildScore(toybox::Screen& s, const ScoreModel& m) {
  if (!m.game) return;
  const auto& g = *m.game;
  const auto b = begin(s);
  cardpanel::title(s, fui::makeRect(b.x, b.y, b.width, 44),
                   m.gameOver ? tr(STR_UI_GAME_OVER_B1DE5C) : tr(STR_HEARTS_ROUND_END));
  char label[100];
  std::snprintf(label, sizeof(label), tr(STR_HEARTS_ROUND_POINTS), g.lastHand.scored[0]);
  dashboard::text(s, fui::makeRect(b.x, b.y + 44, b.width, 30), label, 1, fui::TextAlign::Left);
  const int top = b.y + 88, height = b.height - 150, leftW = b.width * 35 / 100;
  const auto left = fui::makeRect(b.x, top, leftW, height);
  dashboard::frame(s, left);
  cardart::drawSuit(s, fui::makeRect(left.x + (left.width - 90) / 2, left.y + 12, 90, 90), cards::Suit::Hearts, true);
  dashboard::text(s, fui::makeRect(left.x + 8, left.y + 108, left.width - 16, 30), tr(STR_HEARTS_YOUR_POINTS), 1);
  std::snprintf(label, sizeof(label), "%d", g.lastHand.scored[0]);
  dashboard::digits(s, fui::makeRect(left.x + 16, left.y + 142, left.width - 32, std::max(36, int(left.height) - 192)),
                    label);
  std::snprintf(label, sizeof(label), "%s  %d", tr(STR_HEARTS_TOTAL), g.total[0]);
  dashboard::text(s, fui::makeRect(left.x + 8, left.bottom() - 40, left.width - 16, 32), label, 1);
  const auto table = fui::makeRect(left.right() + 10, top, b.width - left.width - 10, height);
  dashboard::frame(s, table);
  const int nameW = table.width * 45 / 100, col = (table.width - nameW) / 2, rowH = (table.height - 40) / 4;
  dashboard::text(s, fui::makeRect(table.x + 10, table.y, nameW - 20, 40), tr(STR_HEARTS_PLAYER), 1,
                  fui::TextAlign::Left);
  dashboard::text(s, fui::makeRect(table.x + nameW, table.y, col, 40), tr(STR_HEARTS_ROUND_LABEL), 0);
  dashboard::text(s, fui::makeRect(table.x + nameW + col, table.y, col, 40), tr(STR_HEARTS_TOTAL), 0);
  int order[] = {0, 1, 2, 3};
  std::sort(order, order + 4,
            [&](int a, int c) { return g.total[a] < g.total[c] || (g.total[a] == g.total[c] && a < c); });
  for (int n = 0; n < 4; ++n) {
    const int seat = order[n];
    const auto row = fui::makeRect(table.x, table.y + 40 + n * rowH, table.width, rowH);
    dashboard::frame(s, row, seat == 0 ? 4 : 1);
    dashboard::text(s, fui::makeRect(row.x + 14, row.y, nameW - 22, rowH), m.seats[seat].name, 1, fui::TextAlign::Left);
    if (g.total[seat] == g.total[order[0]])
      cardpanel::crown(s, fui::makeRect(row.x + nameW - 34, row.y + (rowH - 24) / 2, 24, 24));
    std::snprintf(label, sizeof(label), "+%d", g.lastHand.scored[seat]);
    dashboard::text(s, fui::makeRect(row.x + nameW, row.y, col, rowH), label, 1);
    std::snprintf(label, sizeof(label), "%d", g.total[seat]);
    dashboard::text(s, fui::makeRect(row.x + nameW + col, row.y, col, rowH), label, 1);
  }
  const int w = (b.width - 12) / 2;
  dashboard::button(s, fui::makeRect(b.x, b.bottom() - 50, w, 50),
                    m.gameOver ? tr(STR_UI_NEW_GAME_F6F041) : tr(STR_HEARTS_NEXT_ROUND), ActionButton, ButtonConfirm,
                    true, nullptr, 1);
  dashboard::button(s, fui::makeRect(b.x + w + 12, b.bottom() - 50, w, 50), tr(STR_UI_BACK_TO_MENU_398BBB),
                    ActionButton, ButtonMenu, false, nullptr, 1);
}
namespace {
struct GuideCard {
  Suit suit;
  uint8_t rank;
};
enum class LessonArt { Points, Follow, Pass, Score, Open, Moon };
void helpFan(toybox::Screen& s, fui::Rect r, const GuideCard* values, int count) {
  const int h = std::min(110, int(r.height) - 36 * (count - 1)), w = h * 92 / 122;
  const int x = r.x + (r.width - w) / 2, y = r.y + (r.height - h - 36 * (count - 1)) / 2;
  for (int n = 0; n < count; ++n)
    cardart::drawCardFace(s, fui::makeRect(x, y + n * 36, w, h),
                          cards::faceUp(cards::makeCard(values[n].suit, values[n].rank)), n == count - 1 ? h : 36,
                          cardart::Fan::Down);
}
void lessonArt(toybox::Screen& s, fui::Rect r, LessonArt example) {
  static constexpr GuideCard points[] = {{Suit::Hearts, 0}, {Suit::Spades, 11}};
  static constexpr GuideCard follow[] = {{Suit::Clubs, 2}, {Suit::Clubs, 5}, {Suit::Clubs, 12}};
  static constexpr GuideCard pass[] = {{Suit::Hearts, 1}, {Suit::Spades, 11}, {Suit::Diamonds, 9}};
  static constexpr GuideCard opening[] = {{Suit::Clubs, 1}};
  switch (example) {
    case LessonArt::Points:
      helpFan(s, r, points, 2);
      return;
    case LessonArt::Follow:
      helpFan(s, r, follow, 3);
      return;
    case LessonArt::Pass:
      helpFan(s, r, pass, 3);
      return;
    case LessonArt::Score:
      referenceart::draw(s, r, referenceart::Podium);
      return;
    case LessonArt::Open:
      helpFan(s, r, opening, 1);
      return;
    case LessonArt::Moon: {
      const int y = r.y + (r.height - 184) / 2;
      cardart::drawSuit(s, fui::makeRect(r.x + (r.width - 50) / 2, y, 50, 50), Suit::Hearts, true);
      pixelui::text(s.target(), fui::makeRect(r.x, y + 54, r.width, 26), "13 x 1", 0, fui::Color::Black,
                    fui::TextAlign::Center);
      const int h = 94, w = h * 92 / 122;
      cardart::drawCardFace(s, fui::makeRect(r.x + (r.width - w) / 2, y + 86, w, h),
                            cards::faceUp(cards::makeCard(points[1].suit, points[1].rank)), h);
      return;
    }
  }
}
}  // namespace
int howToPages() { return 4; }
int howToLines(int) { return 4; }
const char* howToTitle(int) { return tr(STR_WORDLE_HELP); }
const char* howToLine(int, int line) {
  switch (line) {
    case 0:
      return tr(STR_HEARTS_PENALTIES);
    case 1:
      return tr(STR_HEARTS_FOLLOW_RULE);
    case 2:
      return tr(STR_HEARTS_PASS_RULE);
    case 3:
      return tr(STR_HEARTS_LOW_RULE);
    default:
      return "";
  }
}
void buildHowTo(toybox::Screen& s, const HowToModel& m) {
  const auto b = begin(s);
  const int page = std::clamp(m.page, 0, howToPages() - 1);
  const bool wide = b.width > b.height;
  static constexpr StrId titles[] = {StrId::STR_CARDS_GUIDE_CARDS, StrId::STR_HEARTS_GUIDE_TRICKS,
                                     StrId::STR_HEARTS_GUIDE_ROUND, StrId::STR_HEARTS_GUIDE_SPECIAL};
  pixelui::text(s.target(), fui::makeRect(b.x, b.y, b.width, 44), I18N.get(titles[page]), 1);
  const auto area = fui::makeRect(b.x, b.y + 54, b.width, b.height - 118);
  if (page == 0)
    cardpanel::deckLesson(s, area, true);
  else {
    static constexpr StrId heads[][2] = {{StrId::STR_HEARTS_AVOID, StrId::STR_HEARTS_FOLLOW},
                                         {StrId::STR_HEARTS_PASS_THREE, StrId::STR_HEARTS_LOW},
                                         {StrId::STR_HEARTS_GUIDE_OPEN, StrId::STR_HEARTS_GUIDE_MOON}};
    static constexpr StrId copy[][2] = {{StrId::STR_HEARTS_PENALTIES, StrId::STR_HEARTS_FOLLOW_RULE},
                                        {StrId::STR_HEARTS_PASS_RULE, StrId::STR_HEARTS_LOW_RULE},
                                        {StrId::STR_HEARTS_GUIDE_OPEN_COPY, StrId::STR_HEARTS_GUIDE_MOON_COPY}};
    static constexpr LessonArt arts[][2] = {{LessonArt::Points, LessonArt::Follow},
                                            {LessonArt::Pass, LessonArt::Score},
                                            {LessonArt::Open, LessonArt::Moon}};
    const int w = wide ? (area.width - 12) / 2 : area.width, h = wide ? area.height : (area.height - 12) / 2;
    for (int n = 0; n < 2; ++n) {
      const auto r = fui::makeRect(area.x + (wide ? n * (w + 12) : 0), area.y + (wide ? 0 : n * (h + 12)), w, h);
      dashboard::frame(s, r, 2);
      pixelui::text(s.target(), fui::makeRect(r.x + 10, r.y + 8, r.width - 20, 34), I18N.get(heads[page - 1][n]), 1);
      const int artW = wide ? 90 : 108;
      lessonArt(s, fui::makeRect(r.x + 10, r.y + 50, artW, r.height - 62), arts[page - 1][n]);
      guideui::text(s, fui::makeRect(r.x + artW + 22, r.y + 52, r.width - artW - 34, r.height - 64),
                    I18N.get(copy[page - 1][n]), 10);
    }
  }
  pageui::build(s, fui::makeRect(b.x, b.bottom() - 48, b.width, 48), page + 1, howToPages(), ActionHelpPage, -1,
                ActionHelpPage, 1);
}
const char* refusalText(const Refusal why) {
  switch (why) {
    case Refusal::NotYourTurn:
      return tr(STR_UI_NOT_YOUR_TURN_YET_637511);
    case Refusal::TwoOfClubsOpens:
      return tr(STR_UI_THE_TWO_OF_CLUBS_OPENS_30E95D);
    case Refusal::MustFollowSuit:
      return tr(STR_UI_YOU_STILL_HOLD_THAT_SUIT_4CFEA2);
    case Refusal::NoPointsFirstTrick:
      return tr(STR_UI_NO_POINTS_ON_TRICK_ONE_4AA5D8);
    case Refusal::HeartsNotBroken:
      return tr(STR_UI_HEARTS_ARE_NOT_BROKEN_2F3EBE);
    case Refusal::JustNo:
      return tr(STR_UI_THE_RULES_REFUSE_THAT_E4FECE);
    case Refusal::Count:
      break;
  }
  return "";
}

// SEEN and UNSEEN rather than OUT and GONE. "QUEEN OUT" meant she had not been
// played while "QUEEN GONE" meant she had -- opposite meanings, one word apart,
// in the smallest type on the screen. These are also short enough to survive
// their box, which the pair before them was not: it was shrunk a cut and then
// truncated anyway, to "QUEEN STILL...".
const char* statusTemplate(const Status which) {
  switch (which) {
    case Status::PickToPass:
      return tr(STR_UI_PICK_THREE_TO_PASS_S_754050);
    case Status::Thinking:
      return tr(STR_UI_S_IS_THINKING_DF70D7);
    case Status::YourLeadFirst:
      return tr(STR_UI_LEAD_THE_TWO_OF_CLUBS_E8406A);
    case Status::YourLead:
      return tr(STR_UI_YOUR_LEAD_47EC69);
    case Status::FollowSuit:
      return tr(STR_UI_FOLLOW_S_3D59EA);
    case Status::TakesIt:
      return tr(STR_HEARTS_TRICK_WINNER);
    case Status::TakesItPoints:
      return tr(STR_HEARTS_TRICK_POINTS);
    case Status::HandOver:
      return tr(STR_UI_HAND_OVER_70413F);
    case Status::GameOver:
      return tr(STR_UI_GAME_OVER_B1DE5C);
    case Status::Count:
      break;
  }
  return "";
}

void longestStatus(const Status which, char* out, const int size) {
  // The widest real substitutions: the longest seat name, the longest suit, the
  // longest pass direction, and a two-digit point count that takes the plural.
  switch (which) {
    case Status::PickToPass:
      std::snprintf(out, size, statusTemplate(which), "%s", tr(STR_UI_ACROSS_A8E6CA));
      return;
    case Status::Thinking:
      std::snprintf(out, size, statusTemplate(which), "%s", tr(STR_UI_NORTH_CF42DF));
      return;
    case Status::FollowSuit:
      std::snprintf(out, size, statusTemplate(which), "%s", tr(STR_UI_DIAMONDS_6CB5FD));
      return;
    case Status::TakesIt:
      std::snprintf(out, size, statusTemplate(which), tr(STR_UI_NORTH_CF42DF));
      return;
    case Status::TakesItPoints:
      std::snprintf(out, size, statusTemplate(which), tr(STR_UI_NORTH_CF42DF), kMoonPoints);
      return;
    default:
      std::snprintf(out, size, "%s", statusTemplate(which));
      return;
  }
}

const char* heartsStateText(const bool broken) {
  return broken ? tr(STR_UI_HEARTS_BROKEN_4F5C5D) : tr(STR_UI_HEARTS_UNBROKEN_E5D772);
}
const char* queenStateText(const bool played) {
  return played ? tr(STR_UI_QUEEN_SEEN_C26E0F) : tr(STR_UI_QUEEN_UNSEEN_45F332);
}

}  // namespace heartsui
