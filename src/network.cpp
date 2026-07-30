#include "anpcpp/network.hpp"

#include "anpcpp/rowsens.hpp"
#include "anpcpp/synthesis.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace anpcpp {

// ---------------------------------------------------------------------------
// NodePrioritizerSlot
// ---------------------------------------------------------------------------

const std::vector<std::string>& NodePrioritizerSlot::alternatives() const {
  return kind == NodePrioritizerKind::Pairwise ? pairwise.alternatives()
                                               : ratings.alternatives();
}

bool NodePrioritizerSlot::empty() const {
  return kind == NodePrioritizerKind::Pairwise ? pairwise.empty()
                                               : ratings.empty();
}

bool NodePrioritizerSlot::has_alternative(const std::string& name) const {
  return kind == NodePrioritizerKind::Pairwise
             ? pairwise.has_alternative(name)
             : ratings.has_alternative(name);
}

void NodePrioritizerSlot::add_alternative(const std::string& name,
                                          bool ignore_existing) {
  if (kind == NodePrioritizerKind::Pairwise) {
    pairwise.add_alternative(name, ignore_existing);
  } else {
    ratings.add_alternative(name, ignore_existing);
  }
}

void NodePrioritizerSlot::remove_alternative(const std::string& name,
                                             bool ignore_missing) {
  if (kind == NodePrioritizerKind::Pairwise) {
    pairwise.remove_alternative(name, ignore_missing);
  } else {
    ratings.remove_alternative(name, ignore_missing);
  }
}

void NodePrioritizerSlot::rename_alternative(const std::string& old_name,
                                             const std::string& new_name) {
  if (kind == NodePrioritizerKind::Pairwise) {
    pairwise.rename_alternative(old_name, new_name);
  } else {
    ratings.rename_alternative(old_name, new_name);
  }
}

Vector NodePrioritizerSlot::priorities() const {
  return kind == NodePrioritizerKind::Pairwise ? pairwise.priorities()
                                               : ratings.priorities();
}

void NodePrioritizerSlot::set_kind(NodePrioritizerKind new_kind) {
  if (kind == new_kind) {
    return;
  }
  const std::vector<std::string> alts = alternatives();
  kind = new_kind;
  if (kind == NodePrioritizerKind::Pairwise) {
    pairwise = PairwiseJudgments(alts);
    ratings = RatingsPrioritizer{};
  } else {
    ratings = RatingsPrioritizer(alts);
    ratings.set_mode(RatingsPrioritizer::Mode::Numeric);
    ratings.set_interpreter(IdentityInterpreter{});
    pairwise = PairwiseJudgments{};
  }
}

// ---------------------------------------------------------------------------
// AnpNode
// ---------------------------------------------------------------------------

AnpNode::AnpNode(AnpNetwork* network, AnpCluster* cluster, std::string name)
    : network_(network), cluster_(cluster), name_(std::move(name)) {}

void AnpNode::connect_to(AnpNode* dest) {
  if (dest == nullptr) {
    throw std::invalid_argument("cannot connect to a null node");
  }
  if (dest->network_ != network_) {
    throw std::invalid_argument("cannot connect nodes from different networks");
  }
  NodePrioritizerSlot& slot = node_prioritizers_[dest->cluster_->name()];
  slot.add_alternative(dest->name_, /*ignore_existing=*/true);
  cluster_->cluster_connect(dest->cluster_);
}

void AnpNode::disconnect_from(AnpNode* dest) {
  if (dest == nullptr) {
    return;
  }
  const auto it = node_prioritizers_.find(dest->cluster_->name());
  if (it == node_prioritizers_.end()) {
    return;
  }
  it->second.remove_alternative(dest->name_, /*ignore_missing=*/true);
  if (it->second.empty()) {
    node_prioritizers_.erase(it);
  }
}

bool AnpNode::is_connected_to(const AnpNode* dest) const {
  if (dest == nullptr) {
    return false;
  }
  const auto it = node_prioritizers_.find(dest->cluster_->name());
  if (it == node_prioritizers_.end()) {
    return false;
  }
  return it->second.has_alternative(dest->name_);
}

bool AnpNode::is_connected_to_cluster(const std::string& cluster_name) const {
  return node_prioritizers_.find(cluster_name) != node_prioritizers_.end();
}

NodePrioritizerSlot* AnpNode::node_prioritizer(const std::string& dest_cluster) {
  const auto it = node_prioritizers_.find(dest_cluster);
  return it == node_prioritizers_.end() ? nullptr : &it->second;
}

const NodePrioritizerSlot* AnpNode::node_prioritizer(
    const std::string& dest_cluster) const {
  const auto it = node_prioritizers_.find(dest_cluster);
  return it == node_prioritizers_.end() ? nullptr : &it->second;
}

PairwiseJudgments* AnpNode::node_pairwise(const std::string& dest_cluster) {
  NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr || slot->kind != NodePrioritizerKind::Pairwise) {
    return nullptr;
  }
  return &slot->pairwise;
}

const PairwiseJudgments* AnpNode::node_pairwise(
    const std::string& dest_cluster) const {
  const NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr || slot->kind != NodePrioritizerKind::Pairwise) {
    return nullptr;
  }
  return &slot->pairwise;
}

RatingsPrioritizer* AnpNode::node_ratings(const std::string& dest_cluster) {
  NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr || slot->kind != NodePrioritizerKind::Ratings) {
    return nullptr;
  }
  return &slot->ratings;
}

const RatingsPrioritizer* AnpNode::node_ratings(
    const std::string& dest_cluster) const {
  const NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr || slot->kind != NodePrioritizerKind::Ratings) {
    return nullptr;
  }
  return &slot->ratings;
}

NodePrioritizerKind AnpNode::node_prioritizer_kind(
    const std::string& dest_cluster) const {
  const NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr) {
    throw std::out_of_range("no prioritizer for destination cluster: " +
                            dest_cluster);
  }
  return slot->kind;
}

void AnpNode::set_node_prioritizer_kind(const std::string& dest_cluster,
                                        NodePrioritizerKind kind) {
  NodePrioritizerSlot* slot = node_prioritizer(dest_cluster);
  if (slot == nullptr) {
    throw std::out_of_range("no prioritizer for destination cluster: " +
                            dest_cluster);
  }
  slot->set_kind(kind);
}

Vector AnpNode::unscaled_column() const {
  Vector rval(network_->nnodes(), 0.0);
  const std::vector<std::string> names = network_->node_names();
  (void)names;
  for (const auto& [cluster_name, slot] : node_prioritizers_) {
    (void)cluster_name;
    if (slot.empty()) {
      continue;
    }
    const Vector local = slot.priorities();
    const auto& alts = slot.alternatives();
    for (std::size_t i = 0; i < alts.size(); ++i) {
      const std::size_t gi = network_->node_index(alts[i]);
      rval[gi] = local[i];
    }
  }
  return rval;
}

AnpNetwork& AnpNode::ensure_subnetwork() {
  if (!subnetwork_) {
    subnetwork_ = std::make_unique<AnpNetwork>(/*create_alts_cluster=*/false);
  }
  return *subnetwork_;
}

// ---------------------------------------------------------------------------
// AnpCluster
// ---------------------------------------------------------------------------

AnpCluster::AnpCluster(AnpNetwork* network, std::string name)
    : network_(network), name_(std::move(name)) {}

AnpNode& AnpCluster::add_node(const std::string& name) {
  if (network_->find_node(name) != nullptr) {
    throw std::invalid_argument("node already exists in network: " + name);
  }
  nodes_.push_back(std::make_unique<AnpNode>(network_, this, name));
  AnpNode* node = nodes_.back().get();
  network_->register_node(node);
  return *node;
}

void AnpCluster::move_node(const std::string& name, std::size_t new_index) {
  std::size_t from = nodes_.size();
  for (std::size_t i = 0; i < nodes_.size(); ++i) {
    if (nodes_[i]->name() == name) {
      from = i;
      break;
    }
  }
  if (from >= nodes_.size()) {
    throw std::invalid_argument("unknown node in cluster: " + name);
  }
  if (new_index >= nodes_.size()) {
    throw std::out_of_range("move_node: new_index out of range");
  }
  if (from == new_index) {
    return;
  }
  auto node = std::move(nodes_[from]);
  nodes_.erase(nodes_.begin() + static_cast<std::ptrdiff_t>(from));
  nodes_.insert(nodes_.begin() + static_cast<std::ptrdiff_t>(new_index),
                std::move(node));
}

AnpNode* AnpCluster::find_node(const std::string& name) {
  for (auto& node : nodes_) {
    if (node->name() == name) {
      return node.get();
    }
  }
  return nullptr;
}

const AnpNode* AnpCluster::find_node(const std::string& name) const {
  for (const auto& node : nodes_) {
    if (node->name() == name) {
      return node.get();
    }
  }
  return nullptr;
}

AnpNode& AnpCluster::node(const std::string& name) {
  AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node in cluster: " + name);
  }
  return *n;
}

const AnpNode& AnpCluster::node(const std::string& name) const {
  const AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node in cluster: " + name);
  }
  return *n;
}

std::vector<std::string> AnpCluster::node_names() const {
  std::vector<std::string> names;
  names.reserve(nodes_.size());
  for (const auto& node : nodes_) {
    names.push_back(node->name());
  }
  return names;
}

std::vector<AnpNode*> AnpCluster::nodes() {
  std::vector<AnpNode*> out;
  out.reserve(nodes_.size());
  for (auto& node : nodes_) {
    out.push_back(node.get());
  }
  return out;
}

std::vector<const AnpNode*> AnpCluster::nodes() const {
  std::vector<const AnpNode*> out;
  out.reserve(nodes_.size());
  for (const auto& node : nodes_) {
    out.push_back(node.get());
  }
  return out;
}

void AnpCluster::cluster_connect(AnpCluster* dest) {
  if (dest == nullptr) {
    throw std::invalid_argument("cannot connect to a null cluster");
  }
  prioritizer_.add_alternative(dest->name(), /*ignore_existing=*/true);
}

// ---------------------------------------------------------------------------
// AnpNetwork
// ---------------------------------------------------------------------------

AnpNetwork::AnpNetwork(bool create_alts_cluster) {
  if (create_alts_cluster) {
    AnpCluster& alts = add_cluster(kDefaultAlternativesCluster);
    alts_cluster_ = &alts;
  }
}

AnpCluster& AnpNetwork::add_cluster(const std::string& name) {
  if (find_cluster(name) != nullptr) {
    throw std::invalid_argument("cluster already exists: " + name);
  }
  clusters_.push_back(std::make_unique<AnpCluster>(this, name));
  return *clusters_.back();
}

AnpCluster* AnpNetwork::find_cluster(const std::string& name) {
  for (auto& c : clusters_) {
    if (c->name() == name) {
      return c.get();
    }
  }
  return nullptr;
}

const AnpCluster* AnpNetwork::find_cluster(const std::string& name) const {
  for (const auto& c : clusters_) {
    if (c->name() == name) {
      return c.get();
    }
  }
  return nullptr;
}

AnpCluster& AnpNetwork::cluster(const std::string& name) {
  AnpCluster* c = find_cluster(name);
  if (c == nullptr) {
    throw std::invalid_argument("unknown cluster: " + name);
  }
  return *c;
}

const AnpCluster& AnpNetwork::cluster(const std::string& name) const {
  const AnpCluster* c = find_cluster(name);
  if (c == nullptr) {
    throw std::invalid_argument("unknown cluster: " + name);
  }
  return *c;
}

void AnpNetwork::set_alternatives_cluster(const std::string& name) {
  alts_cluster_ = &cluster(name);
}

AnpNode& AnpNetwork::add_node(const std::string& cluster_name,
                              const std::string& node_name) {
  return cluster(cluster_name).add_node(node_name);
}

void AnpNetwork::move_node(const std::string& name, std::size_t new_index) {
  AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node: " + name);
  }
  n->cluster()->move_node(name, new_index);
}

AnpNode* AnpNetwork::find_node(const std::string& name) {
  const auto it = node_index_.find(name);
  return it == node_index_.end() ? nullptr : it->second;
}

const AnpNode* AnpNetwork::find_node(const std::string& name) const {
  const auto it = node_index_.find(name);
  return it == node_index_.end() ? nullptr : it->second;
}

AnpNode& AnpNetwork::node(const std::string& name) {
  AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node: " + name);
  }
  return *n;
}

const AnpNode& AnpNetwork::node(const std::string& name) const {
  const AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node: " + name);
  }
  return *n;
}

void AnpNetwork::node_connect(const std::string& src, const std::string& dest) {
  node(src).connect_to(&node(dest));
}

void AnpNetwork::node_disconnect(const std::string& src,
                                 const std::string& dest) {
  node(src).disconnect_from(&node(dest));
}

void AnpNetwork::set_node_comparison(const std::string& wrt_node,
                                     const std::string& a,
                                     const std::string& b,
                                     double value) {
  AnpNode& wrt = node(wrt_node);
  const AnpNode& na = node(a);
  const AnpNode& nb = node(b);
  if (na.cluster()->name() != nb.cluster()->name()) {
    throw std::invalid_argument(
        "node comparisons must be within the same destination cluster");
  }
  // Ensure connections exist so the pairwise table includes both alternatives.
  wrt.connect_to(const_cast<AnpNode*>(&na));
  wrt.connect_to(const_cast<AnpNode*>(&nb));
  if (wrt.node_prioritizer_kind(na.cluster()->name()) ==
      NodePrioritizerKind::Ratings) {
    throw std::logic_error(
        "cannot set pairwise comparison on a ratings prioritizer for cluster " +
        na.cluster()->name());
  }
  PairwiseJudgments* pw = wrt.node_pairwise(na.cluster()->name());
  if (pw == nullptr) {
    throw std::logic_error("missing node pairwise after connect");
  }
  pw->set_comparison(a, b, value);
}

void AnpNetwork::set_cluster_comparison(const std::string& wrt_cluster,
                                        const std::string& a,
                                        const std::string& b,
                                        double value) {
  AnpCluster& wrt = cluster(wrt_cluster);
  AnpCluster& ca = cluster(a);
  AnpCluster& cb = cluster(b);
  wrt.cluster_connect(&ca);
  wrt.cluster_connect(&cb);
  wrt.cluster_pairwise().set_comparison(a, b, value);
}

void AnpNetwork::set_node_prioritizer_kind(const std::string& wrt_node,
                                           const std::string& dest_cluster,
                                           NodePrioritizerKind kind) {
  node(wrt_node).set_node_prioritizer_kind(dest_cluster, kind);
}

void AnpNetwork::set_node_rating(const std::string& wrt_node,
                                 const std::string& alt,
                                 const std::string& category_id) {
  AnpNode& wrt = node(wrt_node);
  AnpNode& dest = node(alt);
  wrt.connect_to(&dest);
  const std::string dest_cluster = dest.cluster()->name();
  if (wrt.node_prioritizer_kind(dest_cluster) != NodePrioritizerKind::Ratings) {
    wrt.set_node_prioritizer_kind(dest_cluster, NodePrioritizerKind::Ratings);
  }
  RatingsPrioritizer* rt = wrt.node_ratings(dest_cluster);
  if (rt == nullptr) {
    throw std::logic_error("missing node ratings after kind switch");
  }
  rt->set_mode(RatingsPrioritizer::Mode::Categorical);
  rt->set_rating(alt, category_id);
}

void AnpNetwork::set_node_rating_value(const std::string& wrt_node,
                                       const std::string& alt,
                                       double raw) {
  AnpNode& wrt = node(wrt_node);
  AnpNode& dest = node(alt);
  wrt.connect_to(&dest);
  const std::string dest_cluster = dest.cluster()->name();
  if (wrt.node_prioritizer_kind(dest_cluster) != NodePrioritizerKind::Ratings) {
    wrt.set_node_prioritizer_kind(dest_cluster, NodePrioritizerKind::Ratings);
  }
  RatingsPrioritizer* rt = wrt.node_ratings(dest_cluster);
  if (rt == nullptr) {
    throw std::logic_error("missing node ratings after kind switch");
  }
  rt->set_mode(RatingsPrioritizer::Mode::Numeric);
  rt->set_value(alt, raw);
}

void AnpNetwork::remove_node(const std::string& name) {
  AnpNode* n = find_node(name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node: " + name);
  }
  // Drop connections that mention this node.
  for (AnpNode* other : nodes()) {
    if (other == n) continue;
    other->disconnect_from(n);
    n->disconnect_from(other);
  }
  AnpCluster* c = n->cluster();
  for (auto it = c->nodes_.begin(); it != c->nodes_.end(); ++it) {
    if (it->get() == n) {
      unregister_node(name);
      node_positions_.erase(name);
      c->nodes_.erase(it);
      return;
    }
  }
}

void AnpNetwork::remove_cluster(const std::string& name) {
  AnpCluster* c = find_cluster(name);
  if (c == nullptr) {
    throw std::invalid_argument("unknown cluster: " + name);
  }
  // Remove nodes first (copies names because remove_node mutates).
  const std::vector<std::string> names = c->node_names();
  for (const std::string& n : names) {
    remove_node(n);
  }
  // Drop this cluster from other clusters' pairwise tables.
  for (auto& other : clusters_) {
    if (other.get() == c) continue;
    other->prioritizer_.remove_alternative(name, /*ignore_missing=*/true);
  }
  if (alts_cluster_ == c) {
    alts_cluster_ = nullptr;
  }
  cluster_positions_.erase(name);
  for (auto it = clusters_.begin(); it != clusters_.end(); ++it) {
    if (it->get() == c) {
      clusters_.erase(it);
      return;
    }
  }
}

void AnpNetwork::rename_node(const std::string& old_name,
                             const std::string& new_name) {
  if (old_name == new_name) {
    return;
  }
  if (new_name.empty()) {
    throw std::invalid_argument("node name must be non-empty");
  }
  AnpNode* n = find_node(old_name);
  if (n == nullptr) {
    throw std::invalid_argument("unknown node: " + old_name);
  }
  if (find_node(new_name) != nullptr) {
    throw std::invalid_argument("node already exists: " + new_name);
  }

  unregister_node(old_name);
  n->name_ = new_name;
  register_node(n);

  const auto pos_it = node_positions_.find(old_name);
  if (pos_it != node_positions_.end()) {
    const auto pos = pos_it->second;
    node_positions_.erase(pos_it);
    node_positions_[new_name] = pos;
  }

  for (AnpNode* other : nodes()) {
    for (auto& [cluster_name, slot] : other->node_prioritizers_) {
      (void)cluster_name;
      if (slot.has_alternative(old_name)) {
        slot.rename_alternative(old_name, new_name);
      }
    }
  }
}

void AnpNetwork::rename_cluster(const std::string& old_name,
                                const std::string& new_name) {
  if (old_name == new_name) {
    return;
  }
  if (new_name.empty()) {
    throw std::invalid_argument("cluster name must be non-empty");
  }
  AnpCluster* c = find_cluster(old_name);
  if (c == nullptr) {
    throw std::invalid_argument("unknown cluster: " + old_name);
  }
  if (find_cluster(new_name) != nullptr) {
    throw std::invalid_argument("cluster already exists: " + new_name);
  }

  c->name_ = new_name;

  for (AnpNode* n : nodes()) {
    auto it = n->node_prioritizers_.find(old_name);
    if (it == n->node_prioritizers_.end()) {
      continue;
    }
    NodePrioritizerSlot slot = std::move(it->second);
    n->node_prioritizers_.erase(it);
    n->node_prioritizers_.emplace(new_name, std::move(slot));
  }

  for (auto& other : clusters_) {
    if (other->prioritizer_.has_alternative(old_name)) {
      other->prioritizer_.rename_alternative(old_name, new_name);
    }
  }

  const auto pos_it = cluster_positions_.find(old_name);
  if (pos_it != cluster_positions_.end()) {
    const auto pos = pos_it->second;
    cluster_positions_.erase(pos_it);
    cluster_positions_[new_name] = pos;
  }
}

void AnpNetwork::clear_subnetwork(const std::string& node_name) {
  node(node_name).subnetwork_.reset();
}

void AnpNetwork::set_cluster_position(const std::string& name,
                                      double x,
                                      double y) {
  (void)cluster(name);  // validate
  cluster_positions_[name] = {x, y};
}

void AnpNetwork::set_node_position(const std::string& name,
                                   double x,
                                   double y) {
  (void)node(name);
  node_positions_[name] = {x, y};
}

bool AnpNetwork::cluster_position(const std::string& name,
                                  double& x,
                                  double& y) const {
  const auto it = cluster_positions_.find(name);
  if (it == cluster_positions_.end()) return false;
  x = it->second.first;
  y = it->second.second;
  return true;
}

bool AnpNetwork::node_position(const std::string& name,
                               double& x,
                               double& y) const {
  const auto it = node_positions_.find(name);
  if (it == node_positions_.end()) return false;
  x = it->second.first;
  y = it->second.second;
  return true;
}

std::size_t AnpNetwork::nnodes() const {
  return node_index_.size();
}

std::vector<std::string> AnpNetwork::cluster_names() const {
  std::vector<std::string> names;
  names.reserve(clusters_.size());
  for (const auto& c : clusters_) {
    names.push_back(c->name());
  }
  return names;
}

std::vector<std::string> AnpNetwork::node_names() const {
  std::vector<std::string> names;
  names.reserve(nnodes());
  for (const auto& c : clusters_) {
    for (const auto& n : c->nodes_) {
      names.push_back(n->name());
    }
  }
  return names;
}

std::vector<AnpCluster*> AnpNetwork::clusters() {
  std::vector<AnpCluster*> out;
  out.reserve(clusters_.size());
  for (auto& c : clusters_) {
    out.push_back(c.get());
  }
  return out;
}

std::vector<const AnpCluster*> AnpNetwork::clusters() const {
  std::vector<const AnpCluster*> out;
  out.reserve(clusters_.size());
  for (const auto& c : clusters_) {
    out.push_back(c.get());
  }
  return out;
}

std::vector<AnpNode*> AnpNetwork::nodes() {
  std::vector<AnpNode*> out;
  out.reserve(nnodes());
  for (auto& c : clusters_) {
    for (auto& n : c->nodes_) {
      out.push_back(n.get());
    }
  }
  return out;
}

std::vector<const AnpNode*> AnpNetwork::nodes() const {
  std::vector<const AnpNode*> out;
  out.reserve(nnodes());
  for (const auto& c : clusters_) {
    for (const auto& n : c->nodes_) {
      out.push_back(n.get());
    }
  }
  return out;
}

void AnpNetwork::register_node(AnpNode* node) {
  node_index_[node->name()] = node;
}

void AnpNetwork::unregister_node(const std::string& name) {
  node_index_.erase(name);
}

std::size_t AnpNetwork::node_index(const std::string& name) const {
  const std::vector<std::string> names = node_names();
  for (std::size_t i = 0; i < names.size(); ++i) {
    if (names[i] == name) {
      return i;
    }
  }
  throw std::invalid_argument("unknown node: " + name);
}

Matrix AnpNetwork::unscaled_supermatrix() const {
  const std::size_t n = nnodes();
  Matrix rval(n, n, 0.0);
  const auto node_list = nodes();
  for (std::size_t col = 0; col < node_list.size(); ++col) {
    const Vector column = node_list[col]->unscaled_column();
    for (std::size_t row = 0; row < n; ++row) {
      rval(row, col) = column[row];
    }
  }
  return rval;
}

Matrix AnpNetwork::cluster_weight_matrix() const {
  const std::size_t nc = nclusters();
  Matrix W(nc, nc, 0.0);
  for (std::size_t col = 0; col < nc; ++col) {
    const AnpCluster* src = clusters_[col].get();
    const PairwiseJudgments& pw = src->cluster_pairwise();
    if (pw.empty()) {
      continue;
    }
    const Vector local = pw.priorities();
    for (std::size_t i = 0; i < pw.size(); ++i) {
      const AnpCluster* dest = find_cluster(pw.alternatives()[i]);
      if (dest == nullptr) {
        continue;
      }
      // Find row index of dest cluster.
      for (std::size_t row = 0; row < nc; ++row) {
        if (clusters_[row].get() == dest) {
          W(row, col) = local[i];
          break;
        }
      }
    }
  }
  return W;
}

Matrix AnpNetwork::scaled_supermatrix() const {
  Matrix rval = unscaled_supermatrix();
  const auto cluster_list = clusters();
  const std::size_t nclusters = cluster_list.size();

  // Columns follow global node order: all nodes in cluster 0, then cluster 1, …
  // For each column (source node), multiply every row in that column by the
  // source cluster's priority of the row's cluster (from cluster pairwise).
  std::size_t col = 0;
  for (std::size_t col_cp = 0; col_cp < nclusters; ++col_cp) {
    const AnpCluster* col_cluster = cluster_list[col_cp];
    const PairwiseJudgments& pw = col_cluster->cluster_pairwise();
    std::map<std::string, double> cluster_pris;
    if (!pw.empty()) {
      const Vector local = pw.priorities();
      for (std::size_t i = 0; i < pw.size(); ++i) {
        cluster_pris[pw.alternatives()[i]] = local[i];
      }
    }

    for (std::size_t col_node = 0; col_node < col_cluster->nnodes();
         ++col_node) {
      (void)col_node;
      std::size_t row = 0;
      for (std::size_t row_cp = 0; row_cp < nclusters; ++row_cp) {
        const AnpCluster* row_cluster = cluster_list[row_cp];
        double priority = 0.0;
        const auto it = cluster_pris.find(row_cluster->name());
        if (it != cluster_pris.end()) {
          priority = it->second;
        }
        for (std::size_t row_node = 0; row_node < row_cluster->nnodes();
             ++row_node) {
          (void)row_node;
          rval(row, col) *= priority;
          ++row;
        }
      }
      ++col;
    }
  }

  // Final step: each column sums to 1 (column stochastic supermatrix).
  column_normalize_inplace(rval);
  return rval;
}

Matrix AnpNetwork::limit_matrix(const LimitMatrixOptions& options) const {
  return calculus_limit(scaled_supermatrix(), options);
}

Vector AnpNetwork::global_priority(const LimitMatrixOptions& options) const {
  return priority_from_limit(limit_matrix(options));
}

bool AnpNetwork::has_subnet() const {
  for (const AnpNode* n : nodes()) {
    if (n->has_subnetwork()) {
      return true;
    }
  }
  return false;
}

AnpNetwork& AnpNetwork::subnet(const std::string& node_name) {
  return node(node_name).ensure_subnetwork();
}

std::vector<std::string> AnpNetwork::alt_names() const {
  if (has_subnet()) {
    std::vector<std::string> rval;
    for (const AnpNode* n : nodes()) {
      if (!n->has_subnetwork()) {
        continue;
      }
      for (const std::string& alt : n->subnetwork()->alt_names()) {
        bool found = false;
        for (const std::string& existing : rval) {
          if (existing == alt) {
            found = true;
            break;
          }
        }
        if (!found) {
          rval.push_back(alt);
        }
      }
    }
    return rval;
  }
  if (alts_cluster_ == nullptr) {
    return {};
  }
  return alts_cluster_->node_names();
}

Vector AnpNetwork::priority(const LimitMatrixOptions& options) const {
  const std::map<std::string, double> scores = priority_map(options);
  const std::vector<std::string> alts = alt_names();
  Vector rval(alts.size(), 0.0);
  for (std::size_t i = 0; i < alts.size(); ++i) {
    const auto it = scores.find(alts[i]);
    rval[i] = it == scores.end() ? 0.0 : it->second;
  }
  return rval;
}

std::map<std::string, double> AnpNetwork::priority_map(
    const LimitMatrixOptions& options) const {
  if (has_subnet()) {
    // Subnetwork model: parent limit priorities weight each subnet's local
    // alternative scores, then synthesis combines them.
    const Vector synth = subnet_synthesize(options);
    const std::vector<std::string> alts = alt_names();
    std::map<std::string, double> out;
    for (std::size_t i = 0; i < alts.size(); ++i) {
      out[alts[i]] = synth[i];
    }
    return out;
  }

  // Flat network: extract alternative entries from the global limit priority.
  const Vector gp = global_priority(options);
  const std::vector<std::string> names = node_names();
  const std::vector<std::string> alts = alt_names();
  std::map<std::string, double> raw;
  double total = 0.0;
  for (const std::string& alt : alts) {
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (names[i] == alt) {
        raw[alt] = gp[i];
        total += gp[i];
        break;
      }
    }
  }
  if (total != 0.0) {
    for (auto& [name, value] : raw) {
      (void)name;
      value /= total;
    }
  }
  return raw;
}

Vector AnpNetwork::subnet_synthesize(const LimitMatrixOptions& options) const {
  return subnet_synthesize_from_global(global_priority(options), options);
}

Vector AnpNetwork::subnet_synthesize_from_global(
    const Vector& gp,
    const LimitMatrixOptions& options) const {
  const std::vector<std::string> names = node_names();
  if (gp.size() != names.size()) {
    throw DimensionError(
        "global priority size does not match network node count");
  }

  std::map<std::string, double> subnet_weights;
  std::map<std::string, std::map<std::string, double>> alt_scores;

  for (const AnpNode* n : nodes()) {
    if (!n->has_subnetwork()) {
      continue;
    }
    double weight = 0.0;
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (names[i] == n->name()) {
        weight = gp[i];
        break;
      }
    }
    subnet_weights[n->name()] = weight;

    std::map<std::string, double> scores =
        n->subnetwork()->priority_map(options);
    if (n->invert()) {
      for (auto& [alt, val] : scores) {
        (void)alt;
        val = 1.0 - val;
      }
    }
    alt_scores[n->name()] = std::move(scores);
  }

  const std::map<std::string, double> combined =
      synthesize(synthesis_, subnet_weights, alt_scores, alt_names());
  const std::vector<std::string> alts = alt_names();
  Vector rval(alts.size(), 0.0);
  for (std::size_t i = 0; i < alts.size(); ++i) {
    const auto it = combined.find(alts[i]);
    rval[i] = it == combined.end() ? 0.0 : it->second;
  }
  if (rval.sum() != 0.0) {
    rval.normalize();
  }
  return rval;
}

std::vector<std::size_t> AnpNetwork::cluster_row_indices(
    const std::string& node_name) const {
  const AnpNode& n = node(node_name);
  std::vector<std::size_t> idxs;
  for (const AnpNode* sib : n.cluster()->nodes()) {
    idxs.push_back(node_index(sib->name()));
  }
  return idxs;
}

std::map<std::string, double> AnpNetwork::priority_map_at_p(
    const std::string& wrt_node,
    double p,
    const P0Mode& p0mode,
    const LimitMatrixOptions& options) const {
  const Matrix W = scaled_supermatrix();
  const std::size_t row = node_index(wrt_node);
  // Empty cluster → full-matrix column renormalization (pyanp default).
  // Cluster-local scaling would pass cluster_row_indices(wrt_node).
  const Matrix adjusted = row_adjust(W, row, p, p0mode, {});
  const Vector gp =
      priority_from_limit(calculus_limit(adjusted, options));

  if (has_subnet()) {
    const Vector synth = subnet_synthesize_from_global(gp, options);
    const std::vector<std::string> alts = alt_names();
    std::map<std::string, double> out;
    for (std::size_t i = 0; i < alts.size(); ++i) {
      out[alts[i]] = synth[i];
    }
    return out;
  }

  // Flat: take alternatives from the adjusted global priority.
  const std::vector<std::string> names = node_names();
  const std::vector<std::string> alts = alt_names();
  std::map<std::string, double> raw;
  double total = 0.0;
  for (const std::string& alt : alts) {
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (names[i] == alt) {
        raw[alt] = gp[i];
        total += gp[i];
        break;
      }
    }
  }
  if (total != 0.0) {
    for (auto& [name, value] : raw) {
      (void)name;
      value /= total;
    }
  }
  return raw;
}

Vector AnpNetwork::priority_at_p(const std::string& wrt_node,
                                 double p,
                                 const P0Mode& p0mode,
                                 const LimitMatrixOptions& options) const {
  const std::map<std::string, double> scores =
      priority_map_at_p(wrt_node, p, p0mode, options);
  const std::vector<std::string> alts = alt_names();
  Vector rval(alts.size(), 0.0);
  for (std::size_t i = 0; i < alts.size(); ++i) {
    const auto it = scores.find(alts[i]);
    rval[i] = it == scores.end() ? 0.0 : it->second;
  }
  return rval;
}

std::vector<InfluenceRawEntry> AnpNetwork::influence_raw(
    const std::string& wrt_node,
    double delta_up,
    double delta_down,
    double p0,
    const LimitMatrixOptions& options) const {
  const P0Mode mode = P0Mode::Direct(p0);
  const auto orig = priority_map_at_p(wrt_node, p0, mode, options);
  const double p_up = std::min(1.0, p0 + delta_up);
  const double p_down = std::max(0.0, p0 - delta_down);
  const auto up = priority_map_at_p(wrt_node, p_up, mode, options);
  const auto down = priority_map_at_p(wrt_node, p_down, mode, options);

  std::vector<InfluenceRawEntry> out;
  for (const std::string& alt : alt_names()) {
    InfluenceRawEntry e;
    e.name = alt;
    e.original = orig.count(alt) ? orig.at(alt) : 0.0;
    e.up_score = up.count(alt) ? up.at(alt) : 0.0;
    e.down_score = down.count(alt) ? down.at(alt) : 0.0;
    e.up_diff = e.up_score - e.original;
    e.down_diff = e.down_score - e.original;
    out.push_back(std::move(e));
  }
  return out;
}

namespace {

bool alt_rank_change(const Vector& a,
                     const Vector& b,
                     int round_to_decimal) {
  const double scale = std::pow(10.0, round_to_decimal);
  const std::size_t n = a.size();
  if (b.size() != n) return true;
  std::vector<std::pair<double, std::size_t>> oa(n), ob(n);
  for (std::size_t i = 0; i < n; ++i) {
    oa[i] = {std::round(a[i] * scale) / scale, i};
    ob[i] = {std::round(b[i] * scale) / scale, i};
  }
  auto rank = [](std::vector<std::pair<double, std::size_t>> v) {
    std::sort(v.begin(), v.end());
    std::vector<double> r(v.size());
    std::size_t i = 0;
    while (i < v.size()) {
      std::size_t j = i + 1;
      while (j < v.size() && v[j].first == v[i].first) ++j;
      const double avg = 0.5 * static_cast<double>(i + j + 1);
      for (std::size_t k = i; k < j; ++k) r[v[k].second] = avg;
      i = j;
    }
    return r;
  };
  const auto ra = rank(oa);
  const auto rb = rank(ob);
  for (std::size_t i = 0; i < n; ++i) {
    if (ra[i] != rb[i]) return true;
  }
  return false;
}

}  // namespace

std::vector<InfluenceRankEntry> AnpNetwork::influence_rank(
    const std::string& wrt_node,
    double error,
    int round_to_decimal,
    const LimitMatrixOptions& options) const {
  const P0Mode mode = P0Mode::Direct(0.5);
  const Vector orig = priority_at_p(wrt_node, 0.5, mode, options);
  const std::vector<std::string> alts = alt_names();

  auto search_upper = [&]() -> double {
    double lower = 0.5;
    double upper = 0.99999;
    Vector lower_pri = priority_at_p(wrt_node, lower, mode, options);
    Vector upper_pri = priority_at_p(wrt_node, upper, mode, options);
    if (!alt_rank_change(lower_pri, upper_pri, round_to_decimal)) return 1.0;
    while ((upper - lower) > error) {
      const double mid = 0.5 * (upper + lower);
      Vector mid_pri = priority_at_p(wrt_node, mid, mode, options);
      if (alt_rank_change(lower_pri, mid_pri, round_to_decimal)) {
        upper = mid;
        upper_pri = std::move(mid_pri);
      } else if (alt_rank_change(mid_pri, upper_pri, round_to_decimal)) {
        lower = mid;
        lower_pri = std::move(mid_pri);
      } else {
        break;
      }
    }
    return (1.0 - upper) / 0.5;
  };
  auto search_lower = [&]() -> double {
    double lower = 0.00001;
    double upper = 0.5;
    Vector lower_pri = priority_at_p(wrt_node, lower, mode, options);
    Vector upper_pri = priority_at_p(wrt_node, upper, mode, options);
    if (!alt_rank_change(lower_pri, upper_pri, round_to_decimal)) return 0.0;
    while ((upper - lower) > error) {
      const double mid = 0.5 * (upper + lower);
      Vector mid_pri = priority_at_p(wrt_node, mid, mode, options);
      if (alt_rank_change(lower_pri, mid_pri, round_to_decimal)) {
        upper = mid;
        upper_pri = std::move(mid_pri);
      } else if (alt_rank_change(mid_pri, upper_pri, round_to_decimal)) {
        lower = mid;
        lower_pri = std::move(mid_pri);
      } else {
        break;
      }
    }
    return lower / 0.5;
  };

  const double score = std::max(search_upper(), search_lower());
  std::vector<InfluenceRankEntry> out;
  out.reserve(alts.size());
  for (std::size_t i = 0; i < alts.size(); ++i) {
    InfluenceRankEntry e;
    e.name = alts[i];
    e.original = i < orig.size() ? orig[i] : 0.0;
    // Same group rank-change distance for each alt (network-level search).
    e.rank_influence = score;
    out.push_back(std::move(e));
  }
  return out;
}

std::vector<InfluenceMarginalEntry> AnpNetwork::influence_marginal_smart(
    const std::string& wrt_node,
    double delta,
    const LimitMatrixOptions& options) const {
  const std::vector<std::string> alts = alt_names();
  const P0Mode base = P0Mode::Direct(0.5);
  const Vector at_p0 = priority_at_p(wrt_node, 0.5, base, options);
  const Vector left =
      priority_at_p(wrt_node, 0.5 - delta, base, options);
  const Vector right =
      priority_at_p(wrt_node, 0.5 + delta, base, options);

  std::vector<InfluenceMarginalEntry> out;
  out.reserve(alts.size());
  for (std::size_t i = 0; i < alts.size(); ++i) {
    const double lval = (at_p0[i] - left[i]) / delta;   // LHS deriv approx
    const double rval = (right[i] - at_p0[i]) / delta;  // RHS
    double p0_smart = 0.5;
    const double denom = lval + rval;
    if (denom != 0.0) {
      p0_smart = lval / denom;
      p0_smart = std::min(0.999, std::max(0.001, p0_smart));
    }
    const Vector at_smart =
        priority_at_p(wrt_node, p0_smart, base, options);
    const Vector at_smart_r =
        priority_at_p(wrt_node, p0_smart + delta, base, options);
    const Vector at_smart_l =
        priority_at_p(wrt_node, p0_smart - delta, base, options);
    const double marg =
        0.5 * ((at_smart_r[i] - at_smart[i]) / delta +
               (at_smart[i] - at_smart_l[i]) / delta);

    InfluenceMarginalEntry e;
    e.name = alts[i];
    e.smart_p0 = p0_smart;
    e.marginal = marg;
    out.push_back(std::move(e));
  }
  return out;
}

}  // namespace anpcpp
