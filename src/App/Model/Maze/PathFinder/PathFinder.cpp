/**
 * @file PathFinder.cpp
 * @brief Implementation of the class PathFinder
 */

#include "PathFinder.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <set>

namespace s21 {

/**
 * @brief Default constructor
 */
PathFinder::PathFinder() : start_({-1, -1}), end_({-1, -1}), path_(), maze_() {}

/**
 * @brief Set the maze
 * @param maze Matrix of the maze
 */
void PathFinder::setMaze(S21Matrix<char>&& maze) {
  reset();
  maze_ = std::move(maze);
}

/**
 * @brief Set the start position of the path
 * @param start Start position
 * @param wRatio Width divide by count of columns
 * @param hRatio Height divide by count of rows
 */
void PathFinder::setStartPosition(Point<float> start, float wRatio,
                                  float hRatio) {
  float y = start.row;
  float x = start.col;

  int rowIndex = y / hRatio;
  int colIndex = x / wRatio;

  Point<int> copy = start_;

  start_ = {colIndex, rowIndex};

  findPath(start_, end_, copy);
}

/**
 * @brief Set the end position of the path
 * @param end End position
 * @param wRatio Width divide by count of columns
 * @param hRatio Height divide by count of rows
 */
void PathFinder::setEndPosition(Point<float> end, float wRatio, float hRatio) {
  float x = end.col;
  float y = end.row;

  int rowIndex = y / hRatio;
  int colIndex = x / wRatio;

  Point<int> copy = end_;

  end_ = {colIndex, rowIndex};

  findPath(end_, start_, copy);
}

/**
 * @brief Wrapper for the A* algorithm function
 * @param first First point
 * @param second Second point
 * @param copy Copy
 */
void PathFinder::findPath(Point<int>& first, Point<int> second,
                          Point<int> copy) {
  if (!maze_.GetRows() || !maze_.GetCols()) return;

  try {
    if (second.col > -1 && second.row > -1) findPathAStar();
  } catch (const OpResult& e) {
    first = copy;
    throw e;
  }
}

/**
 * @brief Reset the path
 */
void PathFinder::reset() {
  start_ = {-1, -1};
  end_ = {-1, -1};
  path_.clear();
}

/**
 * @brief Set the point to the path
 * @param el Point
 * @param config Configuration of the path
 * @param areaSize Size of the area
 */
void PathFinder::setPointToPath(Point<int> el, PathRenderConfig& config,
                                Point<float> areaSize) {
  if (!(el.row > -1 && el.col > -1)) return;

  int width = areaSize.col;
  int height = areaSize.row;

  int rows = maze_.GetRows() / 2;
  int cols = maze_.GetCols() / 2;

  // Calculate the basic cell size
  float baseCellSize = std::min(width / cols, height / rows);

  // Set the size of the square to be 25% of the base cell size
  float squareSize = baseCellSize / 4;

  // Calculate scaling for each dimension
  float scaleFactorX = width / (baseCellSize * cols);
  float scaleFactorY = height / (baseCellSize * rows);

  int colIndex = el.col;
  int rowIndex = el.row;

  // Calculate the central coordinates of a cell with centering
  float centerX = (colIndex + 0.5f) * baseCellSize * scaleFactorX;
  float centerY = (rowIndex + 0.5f) * baseCellSize * scaleFactorY;

  // Create a dot square in the center of the cell
  config.points_.push_back({centerX - squareSize / 2, centerY - squareSize / 2,
                            squareSize, squareSize});
}

/**
 * @brief Get the path
 * @param areaSize Size of the area
 * @return Configuration of the path
 */
PathRenderConfig PathFinder::get(Point<float> areaSize) {
  int rows = maze_.GetRows() / 2;
  int cols = maze_.GetCols() / 2;

  if ((start_.col >= cols || start_.row >= rows) ||
      (end_.col >= cols || end_.row >= rows))
    return {};

  PathRenderConfig config;

  setPointToPath(start_, config, areaSize);
  setPointToPath(end_, config, areaSize);
  fillPath(config, areaSize);

  return config;
}

/**
 * @brief Fill the path
 * @param config Configuration of the path
 * @param areaSize Size of the area
 */
void PathFinder::fillPath(PathRenderConfig& config, Point<float> areaSize) {
  if (path_.empty()) return;

  int rows = maze_.GetRows() / 2;
  int cols = maze_.GetCols() / 2;

  int width = areaSize.col;
  int height = areaSize.row;

  float baseCellSize = std::min(width / cols, height / rows);

  // Calculate scaling factors to maintain aspect ratio within each cell
  float scaleFactorX = width / (baseCellSize * cols);
  float scaleFactorY = height / (baseCellSize * rows);

  for (size_t i = 0; i < path_.size() - 1; ++i) {
    const Point<int> current = path_[i];
    const Point<int> next = path_[i + 1];

    // Calculate center coordinates of current and next cells
    float currentCenterX =
        (current.col / 2 + 0.5f) * baseCellSize * scaleFactorX;
    float currentCenterY =
        (current.row / 2 + 0.5f) * baseCellSize * scaleFactorY;

    float nextCenterX = (next.col / 2 + 0.5f) * baseCellSize * scaleFactorX;
    float nextCenterY = (next.row / 2 + 0.5f) * baseCellSize * scaleFactorY;

    // Draw line connecting centers
    config.path_.push_back(
        {currentCenterX, currentCenterY, nextCenterX, nextCenterY});
  }
}

/**
 * @brief Check if the point is not a wall
 * @param x X coordinate
 * @param y Y coordinate
 * @return True if the point is not a wall
 */
bool PathFinder::isNotWall(int x, int y) {
  const int rows = maze_.GetRows();
  const int cols = maze_.GetCols();

  if ((x >= 0 && x < cols) && (y >= 0 && y < rows)) return maze_(y, x) == '0';

  return false;
}

/**
 * @brief Cost of the path from current to next
 * @param current Current point
 * @param next Next point
 * @return Cost
 */
int PathFinder::calculateG(Point<int> current, Point<int> next) {
  // Cost of horizontal/vertical transition
  int straightCost = 1;

  // Calculating the cost of transition
  int g = 0;
  if (current.col == next.col)
    g = straightCost * std::abs(current.row - next.row);
  else if (current.row == next.row)
    g = straightCost * std::abs(current.col - next.col);

  return g;
}

/**
 * @brief Heuristic function
 * @param point Point
 * @param goal Goal point
 * @return Heuristic
 */
int PathFinder::calculateHeuristic(Point<int> point, Point<int> goal) {
  // Distance in Manhattan
  int dx = std::abs(point.col - goal.col);
  int dy = std::abs(point.row - goal.row);

  return dx + dy;
}

/**
 * @brief Find the path through the A* algorithm
 */
void PathFinder::findPathAStar() {
  Point<int> start = start_;
  Point<int> goal = end_;

  // Multiply by 2 to account for the "extended" dimension (walls take up 2
  // cells)
  start.col *= 2;
  start.row *= 2;
  goal.col *= 2;
  goal.row *= 2;

  // Priority queue for nodes (pair: f-score, point)
  std::priority_queue<std::pair<int, Point<int>>,
                      std::vector<std::pair<int, Point<int>>>,
                      std::greater<std::pair<int, Point<int>>>>
      pq;

  // Set of visited points
  std::set<Point<int>> visited;

  // Map for storing the parent points
  std::map<Point<int>, Point<int>> parent;

  // Initial f-score (g = 0, h = heuristic)
  int f_start = calculateHeuristic(start, goal);
  pq.push({f_start, start});

  visited.insert(start);

  int dx_order[] = {-1, 0, 1, 0};
  int dy_order[] = {0, -1, 0, 1};

  while (!pq.empty()) {
    Point<int> current = pq.top().second;
    pq.pop();

    if (current == goal) {
      reconstructPath(parent, start, goal);
      return;
    }

    for (int i = 0; i < 4; ++i) {
      int dx = dx_order[i];
      int dy = dy_order[i];

      Point<int> next = {current.col + dx, current.row + dy};

      if (isNotWall(next.col, next.row) && !visited.count(next)) {
        int g_new = calculateG(current, next) + calculateG(start, current);
        int h_new = calculateHeuristic(next, goal);
        int f_new = g_new + h_new;

        pq.push({f_new, next});
        visited.insert(next);
        parent[next] = current;
      }
    }
  }

  throw OpResult(
      false, "Path not found. Probably the labyrinth has isolated study areas");
}

/**
 * @brief Reconstruct the path
 * @param parent Parent
 * @param start Start
 * @param end End
 */
void PathFinder::reconstructPath(std::map<Point<int>, Point<int>>& parent,
                                 const Point<int>& start,
                                 const Point<int>& end) {
  path_.clear();

  Point<int> current = end;

  while (current != start) {
    if (!parent.count(current))
      return;  // No path found (e.g., unreachable endpoint)

    path_.push_back(current);
    current = parent.at(current);
  }

  // Add start point to the path
  path_.push_back(start);
}

/**
 * @brief Select the max action
 * @param qTable QTable
 * @param currentPos Current position
 * @return Action
 */
Action PathFinder::selectMaxAction(const QTable& qTable,
                                   Point<int> currentPos) {
  const auto& state = qTable[currentPos.row][currentPos.col];
  auto max = std::max_element(state.qValues.begin(), state.qValues.end());
  return static_cast<Action>(std::distance(state.qValues.begin(), max));
}

/**
 * @brief Select the action
 * @param qTable QTable
 * @param currentPos Current position
 * @param epsilon Epsilon
 * @return Action
 */
Action PathFinder::selectAction(const QTable& qTable, Point<int> currentPos,
                                float epsilon) {
  const auto& state = qTable[currentPos.row][currentPos.col];

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dis(0.0, 1.0);

  float r = dis(gen);

  if (r < epsilon) {
    std::uniform_int_distribution<int> action_dis(0, state.qValues.size() - 1);
    return static_cast<Action>(action_dis(gen));
  } else
    return selectMaxAction(qTable, currentPos);
}

/**
 * @brief Get the next point
 * @param current Current point
 * @param action Action
 * @return Next point
 */
Point<int> PathFinder::getNextPoint(Point<int> current, Action action) {
  Point<int> next{current.col, current.row};
  switch (action) {
    case Action::LEFT:
      next.col--;
      break;
    case Action::UP:
      next.row--;
      break;
    case Action::RIGHT:
      next.col++;
      break;
    case Action::DOWN:
      next.row++;
      break;
  }

  return next;
}

/**
 * @brief Get the reward
 * @param current Current point
 * @param next Next point
 * @param goal Goal point
 * @param done Done
 * @return Reward
 */
float PathFinder::getReward(Point<int> current, Point<int>& next,
                            Point<int> goal, bool& done) {
  if (next == goal) {
    done = true;
    return 10.0f;
  } else if (!isNotWall(next.col, next.row)) {
    done = true;
    next = current;
    return -10.0f;
  } else
    return -0.1f;
}

/**
 * @brief Update the QTable
 * @param qTable QTable
 * @param currentState Current state
 * @param action Action
 * @param next Next state
 * @param reward Reward
 * @param alpha Alpha
 * @param gamma Gamma
 */
void PathFinder::updateQTable(QTable& qTable, Point<int> currentState,
                              Action action, Point<int> next, float reward,
                              float alpha, float gamma) {
  auto& state = qTable[currentState.row][currentState.col];
  int index = static_cast<int>(action);

  auto& nextState = qTable[next.row][next.col];
  float maxQNext =
      *std::max_element(nextState.qValues.begin(), nextState.qValues.end());

  float tg_target = reward + gamma * maxQNext;
  float tg_error = tg_target - state.qValues[index];
  qTable[currentState.row][currentState.col].qValues[index] += alpha * tg_error;
}

/**
 * @brief Get the episodes count
 * @return Episodes count
 */
int PathFinder::getEpisodesCount() {
  int rows = maze_.GetRows() / 2;
  int cols = maze_.GetCols() / 2;

  if (cols > rows) rows = cols;

  if (rows <= 30) return static_cast<float>(rows) * 1.55f * 100.0f;

  if (rows > 40) return rows * 2 * 100 + 500;

  return rows * 2 * 100;
}

/**
 * @brief Find the path through the Q-Learning algorithm
 * @param start Start position
 * @param goal Goal position
 * @return Operation result
 */
OpResult PathFinder::QPathFinding(Point<int> start, Point<int> goal) {
  int rows = maze_.GetRows() / 2;
  int cols = maze_.GetCols() / 2;

  if (start.col < 0 || start.col >= cols || start.row < 0 ||
      start.row >= rows || goal.col < 0 || goal.col >= cols || goal.row < 0 ||
      goal.row >= rows)
    return {false, "Incorrect point"};

  QTable qTable(maze_.GetRows(),
                std::vector<QActions>(maze_.GetCols(), QActions()));

  start_ = start;
  end_ = goal;

  start.col *= 2;
  start.row *= 2;
  goal.col *= 2;
  goal.row *= 2;

  float alpha = 0.9f;
  float gamma = 0.98f;

  float e_initial = 1.0f;
  float decay_rate = 0.01f;
  float epsilon = 0.0f;

  int numEpisodes = getEpisodesCount();

  for (int episode = 0; episode < numEpisodes; episode++) {
    Point<int> currentState = start;

    bool done = false;

    while (!done) {
      Action action = selectAction(qTable, currentState, epsilon);

      Point<int> next = getNextPoint(currentState, action);

      float reward = getReward(currentState, next, goal, done);

      updateQTable(qTable, currentState, action, next, reward, alpha, gamma);

      currentState = next;
    }

    epsilon = e_initial * exp(-decay_rate * episode);
  }

  return buildQPath(qTable);
}

/**
 * @brief Build the path
 * @param qTable QTable
 * @return Operation result
 */
OpResult PathFinder::buildQPath(const QTable& qTable) {
  int count = 0;

  Point<int> current = start_;
  Point<int> end = end_;

  end.col *= 2;
  end.row *= 2;

  current.col *= 2;
  current.row *= 2;

  std::map<Point<int>, Point<int>> parent;

  while (current != end) {
    Action action = selectMaxAction(qTable, current);

    Point<int> next = getNextPoint(current, action);

    parent[next] = current;

    current = next;

    if (count++ > 40000)
      return {
          false,
          "Path not found. Probably the labyrinth has isolated study areas"};
  }

  reconstructPath(parent, {start_.col * 2, start_.row * 2}, end);

  return {true, ""};
}
}  // namespace s21