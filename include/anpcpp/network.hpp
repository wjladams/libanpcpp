/**
 * @file network.hpp
 * @brief ANP network model: clusters, nodes, connections, and calculations.
 */

#pragma once

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "anpcpp/limit_matrix.hpp"
#include "anpcpp/matrix.hpp"
#include "anpcpp/multiuser.hpp"
#include "anpcpp/pairwise.hpp"
#include "anpcpp/ratings.hpp"
#include "anpcpp/rowsens.hpp"
#include "anpcpp/synthesis.hpp"

namespace anpcpp {

class AnpNetwork;
class AnpCluster;

/** @brief Active prioritizer kind for a node → destination-cluster link. */
enum class NodePrioritizerKind { Pairwise, Ratings };

/**
 * @brief Stores either pairwise or ratings judgments for one dest cluster.
 *
 * @c pairwise / @c ratings hold the *effective* judgments used by calc.
 * Per-user tables live in @c user_pairwise / @c user_ratings; call
 * @ref AnpNetwork::rebuild_effective_judgments after edits or scope changes.
 */
struct NodePrioritizerSlot {
  NodePrioritizerKind kind = NodePrioritizerKind::Pairwise;
  PairwiseJudgments pairwise;
  RatingsPrioritizer ratings;
  /** Per-participant pairwise (same alternatives as @c pairwise). */
  std::map<std::string, PairwiseJudgments> user_pairwise;
  /**
   * Per-participant ratings votes. Shared scale (categories / interpreter /
   * mode) is kept on @c ratings and synced into user tables when the scale
   * changes.
   */
  std::map<std::string, RatingsPrioritizer> user_ratings;

  /** @return Alternative names from the active prioritizer. */
  [[nodiscard]] const std::vector<std::string>& alternatives() const;
  /** @return True if the active prioritizer has no alternatives. */
  [[nodiscard]] bool empty() const;
  /** @return True if @p name is an alternative in the active prioritizer. */
  [[nodiscard]] bool has_alternative(const std::string& name) const;
  /** @brief Adds @p name to the active prioritizer and all user tables. */
  void add_alternative(const std::string& name, bool ignore_existing = false);
  /** @brief Removes @p name from the active prioritizer and all user tables. */
  void remove_alternative(const std::string& name,
                          bool ignore_missing = false);
  /** @brief Renames an alternative in the active prioritizer and user tables. */
  void rename_alternative(const std::string& old_name,
                          const std::string& new_name);
  /** @return Local priorities for the unscaled column. */
  [[nodiscard]] Vector priorities() const;
  /**
   * @brief Switches kind, preserving the alternative name list.
   *
   * Pairwise is reset to an identity diagonal; ratings start empty with
   * numeric + Identity interpreter. User maps for the new kind are ensured
   * empty shells matching alternatives.
   */
  void set_kind(NodePrioritizerKind new_kind);

  /** @brief Ensures a user pairwise table exists (identity) for @p user_id. */
  PairwiseJudgments& ensure_user_pairwise(const std::string& user_id);
  /** @brief Ensures a user ratings table exists for @p user_id. */
  RatingsPrioritizer& ensure_user_ratings(const std::string& user_id);
  /** @brief Syncs categories/interpreter/mode from @c ratings into all users. */
  void sync_ratings_scale_to_users();
};

/**
 * @brief A decision element within a cluster (may own a subnetwork).
 */
class AnpNode {
public:
  /**
   * @brief Constructs a node (normally called via AnpCluster::add_node).
   * @param network Owning network.
   * @param cluster Parent cluster.
   * @param name Node name (unique within the network).
   */
  AnpNode(AnpNetwork* network, AnpCluster* cluster, std::string name);

  /** @return Node name. */
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  /** @return Optional description text. */
  [[nodiscard]] const std::string& description() const noexcept {
    return description_;
  }
  /** @brief Sets the node description. */
  void set_description(std::string description) {
    description_ = std::move(description);
  }

  /** @return Parent cluster. */
  [[nodiscard]] AnpCluster* cluster() const noexcept { return cluster_; }
  /** @return Owning network. */
  [[nodiscard]] AnpNetwork* network() const noexcept { return network_; }

  /**
   * @return True if this node's priority is inverted when synthesizing upward.
   */
  [[nodiscard]] bool invert() const noexcept { return invert_; }
  /** @brief Sets invert flag for subnet synthesis. */
  void set_invert(bool value) { invert_ = value; }

  /** @brief Creates node-to-node connection (and cluster-level link). */
  void connect_to(AnpNode* dest);
  /** @brief Removes connection to @p dest. */
  void disconnect_from(AnpNode* dest);
  /** @return True if connected to @p dest. */
  [[nodiscard]] bool is_connected_to(const AnpNode* dest) const;
  /** @return True if connected to any node in @p cluster_name. */
  [[nodiscard]] bool is_connected_to_cluster(const std::string& cluster_name) const;

  /**
   * @brief Pairwise judgments of this node w.r.t. nodes in @p dest_cluster.
   * @return nullptr if no link, or if the link uses ratings.
   */
  [[nodiscard]] PairwiseJudgments* node_pairwise(const std::string& dest_cluster);
  /** @brief Const overload of @ref node_pairwise. */
  [[nodiscard]] const PairwiseJudgments* node_pairwise(
      const std::string& dest_cluster) const;

  /**
   * @brief Ratings judgments of this node w.r.t. nodes in @p dest_cluster.
   * @return nullptr if no link, or if the link uses pairwise.
   */
  [[nodiscard]] RatingsPrioritizer* node_ratings(const std::string& dest_cluster);
  /** @brief Const overload of @ref node_ratings. */
  [[nodiscard]] const RatingsPrioritizer* node_ratings(
      const std::string& dest_cluster) const;

  /**
   * @brief Prioritizer kind for @p dest_cluster.
   * @throws std::out_of_range if there is no link to that cluster.
   */
  [[nodiscard]] NodePrioritizerKind node_prioritizer_kind(
      const std::string& dest_cluster) const;

  /**
   * @brief Sets prioritizer kind for an existing link to @p dest_cluster.
   * @throws std::out_of_range if there is no link to that cluster.
   */
  void set_node_prioritizer_kind(const std::string& dest_cluster,
                                 NodePrioritizerKind kind);

  /**
   * @brief Mutable prioritizer slot for @p dest_cluster, if present.
   * @return nullptr if there is no link to that cluster.
   */
  [[nodiscard]] NodePrioritizerSlot* node_prioritizer(
      const std::string& dest_cluster);
  /** @brief Const overload of @ref node_prioritizer. */
  [[nodiscard]] const NodePrioritizerSlot* node_prioritizer(
      const std::string& dest_cluster) const;

  /**
   * @brief Column of the unscaled supermatrix for this node.
   * @return Vector indexed by global node order.
   */
  [[nodiscard]] Vector unscaled_column() const;

  /** @return Owned subnetwork, or nullptr. */
  [[nodiscard]] AnpNetwork* subnetwork() const noexcept {
    return subnetwork_.get();
  }
  /** @return True if a subnetwork exists. */
  [[nodiscard]] bool has_subnetwork() const noexcept {
    return subnetwork_ != nullptr;
  }
  /** @brief Creates subnetwork if absent and returns it. */
  AnpNetwork& ensure_subnetwork();

private:
  friend class AnpNetwork;
  friend class AnpCluster;

  AnpNetwork* network_ = nullptr;
  AnpCluster* cluster_ = nullptr;
  std::string name_;
  std::string description_;
  bool invert_ = false;
  std::map<std::string, NodePrioritizerSlot> node_prioritizers_;
  std::unique_ptr<AnpNetwork> subnetwork_;
};

/**
 * @brief A group of nodes (and cluster-level pairwise comparisons).
 */
class AnpCluster {
public:
  /**
   * @brief Constructs a cluster (normally via AnpNetwork::add_cluster).
   * @param network Owning network.
   * @param name Cluster name (unique within the network).
   */
  AnpCluster(AnpNetwork* network, std::string name);

  /** @return Cluster name. */
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  /** @return Optional description. */
  [[nodiscard]] const std::string& description() const noexcept {
    return description_;
  }
  /** @brief Sets cluster description. */
  void set_description(std::string description) {
    description_ = std::move(description);
  }

  /** @return Owning network. */
  [[nodiscard]] AnpNetwork* network() const noexcept { return network_; }
  /** @return Number of nodes in this cluster. */
  [[nodiscard]] std::size_t nnodes() const noexcept { return nodes_.size(); }

  /**
   * @brief Adds a node to this cluster.
   * @throws std::runtime_error if the name already exists in the network.
   */
  AnpNode& add_node(const std::string& name);

  /**
   * @brief Reorders a node within this cluster (preserves pairwise data).
   * @param name Node to move.
   * @param new_index Target 0-based index.
   */
  void move_node(const std::string& name, std::size_t new_index);

  /** @return Pointer to node or nullptr. */
  [[nodiscard]] AnpNode* find_node(const std::string& name);
  /** @brief Const overload of @ref find_node. */
  [[nodiscard]] const AnpNode* find_node(const std::string& name) const;
  /**
   * @return Reference to named node.
   * @throws std::out_of_range if not found.
   */
  [[nodiscard]] AnpNode& node(const std::string& name);
  /** @brief Const overload of @ref node. */
  [[nodiscard]] const AnpNode& node(const std::string& name) const;

  /** @return Names of nodes in cluster order. */
  [[nodiscard]] std::vector<std::string> node_names() const;
  /** @return Mutable pointers to nodes in order. */
  [[nodiscard]] std::vector<AnpNode*> nodes();
  /** @return Const node pointers in order. */
  [[nodiscard]] std::vector<const AnpNode*> nodes() const;

  /** @brief Connects this cluster to @p dest (enables cluster pairwise). */
  void cluster_connect(AnpCluster* dest);
  /** @return Cluster-level pairwise judgments (effective). */
  [[nodiscard]] PairwiseJudgments& cluster_pairwise() { return prioritizer_; }
  /** @brief Const overload of @ref cluster_pairwise. */
  [[nodiscard]] const PairwiseJudgments& cluster_pairwise() const {
    return prioritizer_;
  }
  /** @return Per-user cluster pairwise tables. */
  [[nodiscard]] std::map<std::string, PairwiseJudgments>& user_cluster_pairwise() {
    return user_prioritizers_;
  }
  /** @brief Const overload. */
  [[nodiscard]] const std::map<std::string, PairwiseJudgments>&
  user_cluster_pairwise() const {
    return user_prioritizers_;
  }
  /** @brief Ensures identity pairwise for @p user_id matching cluster alts. */
  PairwiseJudgments& ensure_user_cluster_pairwise(const std::string& user_id);

private:
  friend class AnpNetwork;
  friend class AnpNode;

  AnpNetwork* network_ = nullptr;
  std::string name_;
  std::string description_;
  std::vector<std::unique_ptr<AnpNode>> nodes_;
  PairwiseJudgments prioritizer_;
  std::map<std::string, PairwiseJudgments> user_prioritizers_;
};

/**
 * @brief Root ANP model: structure, judgments, supermatrices, and priorities.
 */
class AnpNetwork {
public:
  /** Default name for the alternatives cluster when auto-created. */
  static constexpr const char* kDefaultAlternativesCluster = "Alternatives";

  /**
   * @brief Constructs an empty network.
   * @param create_alts_cluster If true, creates an "Alternatives" cluster.
   */
  explicit AnpNetwork(bool create_alts_cluster = true);

  AnpNetwork(const AnpNetwork&) = delete;
  AnpNetwork& operator=(const AnpNetwork&) = delete;
  /** @brief Move constructor. */
  AnpNetwork(AnpNetwork&&) noexcept = default;
  /** @brief Move assignment. */
  AnpNetwork& operator=(AnpNetwork&&) noexcept = default;

  /**
   * @brief Adds a cluster.
   * @throws std::runtime_error if the name already exists.
   */
  AnpCluster& add_cluster(const std::string& name);
  /** @return Cluster pointer or nullptr. */
  [[nodiscard]] AnpCluster* find_cluster(const std::string& name);
  /** @brief Const overload of @ref find_cluster. */
  [[nodiscard]] const AnpCluster* find_cluster(const std::string& name) const;
  /**
   * @return Named cluster.
   * @throws std::out_of_range if not found.
   */
  [[nodiscard]] AnpCluster& cluster(const std::string& name);
  /** @brief Const overload of @ref cluster. */
  [[nodiscard]] const AnpCluster& cluster(const std::string& name) const;

  /** @return The designated alternatives cluster, if any. */
  [[nodiscard]] AnpCluster* alternatives_cluster() const noexcept {
    return alts_cluster_;
  }
  /** @brief Marks @p name as the alternatives cluster. */
  void set_alternatives_cluster(const std::string& name);

  /** @brief Adds a node to @p cluster_name. */
  AnpNode& add_node(const std::string& cluster_name, const std::string& node_name);
  /** @brief Reorders a node within its cluster. */
  void move_node(const std::string& name, std::size_t new_index);
  /** @return Node pointer or nullptr. */
  [[nodiscard]] AnpNode* find_node(const std::string& name);
  /** @brief Const overload of @ref find_node. */
  [[nodiscard]] const AnpNode* find_node(const std::string& name) const;
  /** @return Named node. @throws std::out_of_range if not found. */
  [[nodiscard]] AnpNode& node(const std::string& name);
  /** @brief Const overload of @ref node. */
  [[nodiscard]] const AnpNode& node(const std::string& name) const;

  /** @brief Connects two nodes by name. */
  void node_connect(const std::string& src, const std::string& dest);
  /** @brief Disconnects two nodes. */
  void node_disconnect(const std::string& src, const std::string& dest);
  /**
   * @brief Sets node pairwise comparison (w.r.t. @p wrt_node's cluster links).
   * @throws std::logic_error if the link uses ratings.
   */
  void set_node_comparison(const std::string& wrt_node,
                           const std::string& a,
                           const std::string& b,
                           double value);
  /** @brief Sets cluster-level pairwise comparison. */
  void set_cluster_comparison(const std::string& wrt_cluster,
                              const std::string& a,
                              const std::string& b,
                              double value);

  /**
   * @brief Sets prioritizer kind for @p wrt_node → @p dest_cluster.
   * @throws std::out_of_range if there is no link to that cluster.
   */
  void set_node_prioritizer_kind(const std::string& wrt_node,
                                 const std::string& dest_cluster,
                                 NodePrioritizerKind kind);

  /**
   * @brief Sets a categorical rating (ensures connection; switches to ratings).
   */
  void set_node_rating(const std::string& wrt_node,
                       const std::string& alt,
                       const std::string& category_id);

  /**
   * @brief Sets a numeric rating value (ensures connection; switches to ratings).
   */
  void set_node_rating_value(const std::string& wrt_node,
                             const std::string& alt,
                             double raw);

  /** @brief Removes a node and its connections. */
  void remove_node(const std::string& name);
  /** @brief Removes a cluster and its nodes. */
  void remove_cluster(const std::string& name);
  /**
   * @brief Renames a node network-wide (index, judgments, layout).
   * @throws std::invalid_argument if unknown or @p new_name already exists.
   */
  void rename_node(const std::string& old_name, const std::string& new_name);
  /**
   * @brief Renames a cluster network-wide (judgment keys, layout).
   * @throws std::invalid_argument if unknown or @p new_name already exists.
   */
  void rename_cluster(const std::string& old_name, const std::string& new_name);
  /** @brief Clears a node's subnetwork. */
  void clear_subnetwork(const std::string& node_name);

  /** @return Optional display name (empty means UI default such as "Root"). */
  [[nodiscard]] const std::string& name() const noexcept { return name_; }
  /** @brief Sets the network display name. */
  void set_name(std::string name) { name_ = std::move(name); }

  /** @return Optional description text. */
  [[nodiscard]] const std::string& description() const noexcept {
    return description_;
  }
  /** @brief Sets the network description. */
  void set_description(std::string description) {
    description_ = std::move(description);
  }

  /** @return Synthesis options for subnetwork score combination. */
  [[nodiscard]] const SynthesisOptions& synthesis_options() const noexcept {
    return synthesis_;
  }
  /** @brief Sets synthesis options. */
  void set_synthesis_options(SynthesisOptions options) {
    synthesis_ = std::move(options);
  }

  /**
   * @return Limit-matrix options used as the network default for calculations.
   *
   * Callers that want the network setting should pass
   * @ref limit_matrix_options to @ref limit_matrix / @ref global_priority /
   * sensitivity APIs. Explicit @c LimitMatrixOptions arguments still override.
   */
  [[nodiscard]] const LimitMatrixOptions& limit_matrix_options() const noexcept {
    return limit_options_;
  }
  /** @brief Sets the network default limit-matrix options. */
  void set_limit_matrix_options(LimitMatrixOptions options) {
    limit_options_ = std::move(options);
  }

  /** @brief Stores GUI layout hint (ignored by calculations). */
  void set_cluster_position(const std::string& name, double x, double y);
  /** @brief Stores node canvas position (ignored by calculations). */
  void set_node_position(const std::string& name, double x, double y);
  /**
   * @brief Retrieves cluster layout position.
   * @return False if no position stored.
   */
  [[nodiscard]] bool cluster_position(const std::string& name,
                                      double& x,
                                      double& y) const;
  /**
   * @brief Retrieves node layout position.
   * @return False if no position stored.
   */
  [[nodiscard]] bool node_position(const std::string& name,
                                   double& x,
                                   double& y) const;

  /** @return Total node count in this network. */
  [[nodiscard]] std::size_t nnodes() const;
  /** @return Cluster count. */
  [[nodiscard]] std::size_t nclusters() const noexcept {
    return clusters_.size();
  }
  /** @return All cluster names. */
  [[nodiscard]] std::vector<std::string> cluster_names() const;
  /** @return All node names (global order). */
  [[nodiscard]] std::vector<std::string> node_names() const;
  /** @return Cluster pointers. */
  [[nodiscard]] std::vector<AnpCluster*> clusters();
  /** @return Const cluster pointers. */
  [[nodiscard]] std::vector<const AnpCluster*> clusters() const;
  /** @return Node pointers in global order. */
  [[nodiscard]] std::vector<AnpNode*> nodes();
  /** @return Const node pointers. */
  [[nodiscard]] std::vector<const AnpNode*> nodes() const;

  /** @return Unscaled supermatrix (columns = global node priorities). */
  [[nodiscard]] Matrix unscaled_supermatrix() const;
  /** @return Cluster weight matrix. */
  [[nodiscard]] Matrix cluster_weight_matrix() const;
  /** @return Cluster-weighted, column-normalized supermatrix. */
  [[nodiscard]] Matrix scaled_supermatrix() const;
  /** @return Limit matrix from scaled supermatrix. */
  [[nodiscard]] Matrix limit_matrix(
      const LimitMatrixOptions& options = {}) const;
  /** @return Global priority vector from the limit matrix. */
  [[nodiscard]] Vector global_priority(
      const LimitMatrixOptions& options = {}) const;

  /** @return True if any node has a subnetwork. */
  [[nodiscard]] bool has_subnet() const;
  /**
   * @return Subnetwork owned by @p node_name.
   * @throws std::out_of_range if the node has no subnetwork.
   */
  AnpNetwork& subnet(const std::string& node_name);
  /** @return Alternative names (from subnetworks or flat alts cluster). */
  [[nodiscard]] std::vector<std::string> alt_names() const;
  /**
   * @return Alternative priority vector (synthesized when subnetworks exist).
   */
  [[nodiscard]] Vector priority(const LimitMatrixOptions& options = {}) const;
  /** @return Alternative scores as a name -> value map. */
  [[nodiscard]] std::map<std::string, double> priority_map(
      const LimitMatrixOptions& options = {}) const;

  /**
   * @brief Alternative scores after ANP row sensitivity at parameter @p p.
   *
   * Pipeline: scaled SM → row_adjust → limit → global priorities; if this
   * network has subnetworks, those global priorities weight subnet synthesis
   * (same as @ref priority_map, but with the adjusted parent limit).
   */
  [[nodiscard]] std::map<std::string, double> priority_map_at_p(
      const std::string& wrt_node,
      double p,
      const P0Mode& p0mode = P0Mode::Direct(0.5),
      const LimitMatrixOptions& options = {}) const;

  /** @brief Ordered alternative scores from @ref priority_map_at_p. */
  [[nodiscard]] Vector priority_at_p(
      const std::string& wrt_node,
      double p,
      const P0Mode& p0mode = P0Mode::Direct(0.5),
      const LimitMatrixOptions& options = {}) const;

  /**
   * @brief Raw fixed-distance influence on alternative scores (incl. subnets).
   */
  [[nodiscard]] std::vector<InfluenceRawEntry> influence_raw(
      const std::string& wrt_node,
      double delta_up = 0.1,
      double delta_down = 0.1,
      double p0 = 0.5,
      const LimitMatrixOptions& options = {}) const;

  /**
   * @brief Rank influence score for each node (row) in this network.
   *
   * No Wrt selector: for each node, measures how far \(p\) must move from 0.5
   * before alternative rankings change when that node's row is adjusted.
   */
  [[nodiscard]] std::vector<InfluenceRankEntry> influence_rank(
      double error = 1e-5,
      int round_to_decimal = 5,
      const LimitMatrixOptions& options = {}) const;

  /**
   * @brief Smart-\(p_0\) marginal influence score for each node (row).
   *
   * Per row: L1 of absolute per-alternative smart marginals; representative
   * smart \(p_0\) is that of the alternative with largest absolute marginal.
   */
  [[nodiscard]] std::vector<InfluenceMarginalEntry> influence_marginal_smart(
      double delta = 1e-6,
      const LimitMatrixOptions& options = {}) const;

  /**
   * @brief Fixed-distance total influence for each node (row).
   *
   * pyanp `influence_fixed` Total / Max Alt Change: L1 and max of absolute
   * alternative-score diffs after moving \(p\) from 0.5 by @p delta.
   */
  [[nodiscard]] std::vector<InfluenceTotalEntry> influence_total(
      double delta = 0.25,
      const LimitMatrixOptions& options = {}) const;

  // --- Multi-user judgments -------------------------------------------------

  /** @return Model participants (root network owns the roster). */
  [[nodiscard]] const std::vector<JudgmentParticipant>& participants() const {
    return participants_;
  }
  /** @return Mutable participants list. */
  [[nodiscard]] std::vector<JudgmentParticipant>& participants() {
    return participants_;
  }
  /** @return Named judgment groups. */
  [[nodiscard]] const std::vector<JudgmentGroup>& judgment_groups() const {
    return groups_;
  }
  /** @return Mutable groups. */
  [[nodiscard]] std::vector<JudgmentGroup>& judgment_groups() { return groups_; }
  /** @return Current judgment session scope. */
  [[nodiscard]] const JudgmentSession& judgment_session() const {
    return session_;
  }
  /** @brief Sets session scope (does not rebuild; call @ref rebuild_effective_judgments). */
  void set_judgment_session(JudgmentSession session) {
    session_ = std::move(session);
  }

  /**
   * @brief Adds a participant if @p id is new.
   * @return Reference to the participant.
   */
  JudgmentParticipant& add_participant(std::string id,
                                       std::string name,
                                       std::string email = {});
  /** @brief Removes participant and their judgment tables. */
  void remove_participant(const std::string& id);
  /** @return Participant pointer or nullptr. */
  [[nodiscard]] JudgmentParticipant* find_participant(const std::string& id);
  /** @brief Const overload. */
  [[nodiscard]] const JudgmentParticipant* find_participant(
      const std::string& id) const;

  JudgmentGroup& add_judgment_group(std::string id,
                                    std::string name,
                                    std::vector<std::string> member_ids = {});
  void remove_judgment_group(const std::string& id);
  [[nodiscard]] JudgmentGroup* find_judgment_group(const std::string& id);
  [[nodiscard]] const JudgmentGroup* find_judgment_group(
      const std::string& id) const;

  /**
   * @brief Participant ids contributing under the current session.
   *
   * Average → all participants; Participant → that id; Group → members.
   */
  [[nodiscard]] std::vector<std::string> session_member_ids() const;

  /**
   * @brief Rebuilds all effective pairwise/ratings slots from user tables
   *        and the current @ref judgment_session.
   *
   * Call after editing user judgments or changing scope. Subnetworks are
   * rebuilt recursively.
   */
  void rebuild_effective_judgments();

  /**
   * @brief Ensures every participant has user tables on existing links;
   *        migrates legacy effective-only data into @c default if needed.
   */
  void ensure_multiuser_initialized();

  /**
   * @brief Sets node pairwise for @p user_id (creates connections as needed).
   */
  void set_node_comparison_for(const std::string& user_id,
                               const std::string& wrt_node,
                               const std::string& a,
                               const std::string& b,
                               double value);

  /** @brief Sets cluster pairwise for @p user_id. */
  void set_cluster_comparison_for(const std::string& user_id,
                                  const std::string& wrt_cluster,
                                  const std::string& a,
                                  const std::string& b,
                                  double value);

  /** @brief Sets categorical rating for @p user_id. */
  void set_node_rating_for(const std::string& user_id,
                           const std::string& wrt_node,
                           const std::string& alt,
                           const std::string& category_id);

  /** @brief Sets numeric rating for @p user_id. */
  void set_node_rating_value_for(const std::string& user_id,
                                 const std::string& wrt_node,
                                 const std::string& alt,
                                 double raw);

private:
  friend class AnpNode;
  friend class AnpCluster;

  void register_node(AnpNode* node);
  void unregister_node(const std::string& name);
  [[nodiscard]] std::size_t node_index(const std::string& name) const;
  /** @return Global indices of nodes in @p node_name's cluster. */
  [[nodiscard]] std::vector<std::size_t> cluster_row_indices(
      const std::string& node_name) const;

  /**
   * @brief Subnet synthesis using a supplied parent global-priority vector.
   */
  [[nodiscard]] Vector subnet_synthesize_from_global(
      const Vector& gp,
      const LimitMatrixOptions& options) const;

  [[nodiscard]] Vector subnet_synthesize(
      const LimitMatrixOptions& options) const;

  void rebuild_effective_judgments_local();
  void migrate_effective_into_default_user();

  std::vector<std::unique_ptr<AnpCluster>> clusters_;
  AnpCluster* alts_cluster_ = nullptr;
  std::unordered_map<std::string, AnpNode*> node_index_;
  std::string name_;
  std::string description_;
  SynthesisOptions synthesis_;
  LimitMatrixOptions limit_options_;
  std::map<std::string, std::pair<double, double>> cluster_positions_;
  std::map<std::string, std::pair<double, double>> node_positions_;
  std::vector<JudgmentParticipant> participants_;
  std::vector<JudgmentGroup> groups_;
  JudgmentSession session_;
};

}  // namespace anpcpp
