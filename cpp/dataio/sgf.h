#ifndef DATAIO_SGF_H_
#define DATAIO_SGF_H_

#include "../core/global.h"
#include "../core/hash.h"
#include "../dataio/trainingwrite.h"
#include "../game/board.h"
#include "../game/boardhistory.h"

STRUCT_NAMED_TRIPLE(uint8_t,x,uint8_t,y,Player,pla,MoveNoBSize);
STRUCT_NAMED_PAIR(int,x,int,y,XYSize);

struct SgfNode {
  std::map<std::string,std::vector<std::string>>* props;
  MoveNoBSize move;

  SgfNode();
  ~SgfNode();

  SgfNode(const SgfNode& other);
  SgfNode(SgfNode&& other) noexcept;

  SgfNode& operator=(const SgfNode&);
  SgfNode& operator=(SgfNode&&) noexcept;

  bool hasProperty(const char* key) const;
  std::string getSingleProperty(const char* key) const;

  bool hasPlacements() const;
  void accumPlacements(std::vector<Move>& moves, int xSize, int ySize) const;
  void accumMoves(std::vector<Move>& moves, int xSize, int ySize) const;

  Color getPLSpecifiedColor() const;
  Rules getRulesFromRUTagOrFail() const;
};

struct Sgf {
  std::string fileName;
  std::vector<SgfNode*> nodes;
  std::vector<Sgf*> children;
  Hash128 hash;

  Sgf();
  ~Sgf();

  Sgf(const Sgf&) = delete;
  Sgf& operator=(const Sgf&) = delete;

  static Sgf* parse(const std::string& str);
  static Sgf* loadFile(const std::string& file);
  static std::vector<Sgf*> loadFiles(const std::vector<std::string>& files);
  static std::vector<Sgf*> loadSgfsFile(const std::string& file);
  static std::vector<Sgf*> loadSgfsFiles(const std::vector<std::string>& files);

  XYSize getXYSize() const;
  float getKomi() const;
  bool hasRules() const;
  Rules getRulesOrFail() const;

  void getPlacements(std::vector<Move>& moves, int xSize, int ySize) const;
  void getMoves(std::vector<Move>& moves, int xSize, int ySize) const;

  int depth() const;

  struct PositionSample {
    Board board;
    Player nextPla;
    //Prior to using the sample, play these moves on to the board.
    //This provides a little bit of history and context, which can also be relevant for setting up ko prohibitions.
    std::vector<Move> moves;
    //Turn number as of the start of board.
    int initialTurnNumber;
  };

  //Loads SGF all unique positions in ALL branches of that SGF, sampling them with the specified probability.
  //Hashes are used to filter out "identical" positions when loading many files from different SGFs that may have overlapping openings, etc.
  //The hashes are not guaranteed to correspond to position hashes, or anything else external to this function itself.
  //May raise an exception on illegal moves or other SGF issues, only partially appending things on to the boards and hists.
  void loadAllUniquePositions(std::set<Hash128>& uniqueHashes, std::vector<PositionSample>& samples) const;
  //f is allowed to mutate and consume sample.
  void iterAllUniquePositions(std::set<Hash128>& uniqueHashes, std::function<void(PositionSample&)> f) const;

  private:
  void getMovesHelper(std::vector<Move>& moves, int xSize, int ySize) const;


  void iterAllUniquePositionsHelper(
    Board& board, BoardHistory& hist, Player nextPla,
    const Rules& rules, int xSize, int ySize,
    PositionSample& sampleBuf,
    int initialTurnNumber,
    std::set<Hash128>& uniqueHashes,
    std::function<void(PositionSample&)> f
  ) const;
  void samplePositionIfUniqueHelper(
    Board& board, BoardHistory& hist, Player nextPla,
    PositionSample& sampleBuf,
    int initialTurnNumber,
    std::set<Hash128>& uniqueHashes,
    std::function<void(PositionSample&)> f
  ) const;

};

struct CompactSgf {
  std::string fileName;
  SgfNode rootNode;
  std::vector<Move> placements;
  std::vector<Move> moves;
  int xSize;
  int ySize;
  int depth;
  float komi;
  Hash128 hash;

  CompactSgf(const Sgf* sgf);
  CompactSgf(Sgf&& sgf);
  ~CompactSgf();

  CompactSgf(const CompactSgf&) = delete;
  CompactSgf& operator=(const CompactSgf&) = delete;

  static CompactSgf* parse(const std::string& str);
  static CompactSgf* loadFile(const std::string& file);
  static std::vector<CompactSgf*> loadFiles(const std::vector<std::string>& files);

  bool hasRules() const;
  Rules getRulesOrFail() const;
  Rules getRulesOrFailAllowUnspecified(const Rules& defaultRules) const;
  Rules getRulesOrWarn(const Rules& defaultRules, std::function<void(const std::string& msg)> f) const;

  void setupInitialBoardAndHist(const Rules& initialRules, Board& board, Player& nextPla, BoardHistory& hist) const;
  void setupBoardAndHist(const Rules& initialRules, Board& board, Player& nextPla, BoardHistory& hist, int turnNumber) const;
};

namespace WriteSgf {
  //Write an SGF with no newlines to the given ostream.
  //If startTurnIdx >= 0, write a comment in the SGF root node indicating startTurnIdx, so as to
  //indicate the index of the first turn that should be used for training data. (0 means the whole SGF, 1 means skipping black's first move, etc).
  //If valueTargets is not NULL, also write down after each move the MCTS values following that search move.
  void writeSgf(
    std::ostream& out, const std::string& bName, const std::string& wName, const Rules& rules,
    const BoardHistory& hist,
    const FinishedGameData* gameData
  );

  //If hist is a finished game, print the result to out, else do nothing
  void printGameResult(std::ostream& out, const BoardHistory& hist);
}

#endif  // DATAIO_SGF_H_
