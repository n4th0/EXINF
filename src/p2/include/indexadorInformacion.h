#ifndef _INFORMACION_
#define _INFORMACION_

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
using namespace std;

template <typename Key, typename Value, typename Compare = std::less<Key>>
class AVLMap {
  // ── Nodo interno ──────────────────────────────────────────────────────────
  struct Node {
    Key key;
    Value val;
    Node *left = nullptr;
    Node *right = nullptr;
    int height = 1;

    Node(const Key &k, const Value &v) : key(k), val(v) {}
    Node(const Key &k, Value &&v) : key(k), val(std::move(v)) {}
    Node(Key &&k, Value &&v) : key(std::move(k)), val(std::move(v)) {}
  };

  Node *root_ = nullptr;
  size_t size_ = 0;
  Compare cmp_;

  // ── Utilidades de altura / balance ────────────────────────────────────────
  static int height(const Node *n) { return n ? n->height : 0; }
  static int bf(const Node *n) {
    return n ? height(n->left) - height(n->right) : 0;
  }
  static void updateHeight(Node *n) {
    if (n)
      n->height = 1 + std::max(height(n->left), height(n->right));
  }

  // ── Rotaciones ────────────────────────────────────────────────────────────
  static Node *rotR(Node *y) {
    Node *x = y->left;
    Node *T2 = x->right;
    x->right = y;
    y->left = T2;
    updateHeight(y);
    updateHeight(x);
    return x;
  }
  static Node *rotL(Node *x) {
    Node *y = x->right;
    Node *T2 = y->left;
    y->left = x;
    x->right = T2;
    updateHeight(x);
    updateHeight(y);
    return y;
  }
  static Node *balance(Node *n) {
    updateHeight(n);
    int b = bf(n);
    if (b > 1) {
      if (bf(n->left) < 0)
        n->left = rotL(n->left);
      return rotR(n);
    }
    if (b < -1) {
      if (bf(n->right) > 0)
        n->right = rotR(n->right);
      return rotL(n);
    }
    return n;
  }

  // ── Inserción / emplace ───────────────────────────────────────────────────
  // Devuelve {nuevo_root, puntero_al_valor_insertado_o_existente}
  std::pair<Node *, Value *> insert_(Node *n, const Key &k) {
    if (!n) {
      ++size_;
      Node *nd = new Node(k, Value{});
      return {nd, &nd->val};
    }
    if (cmp_(k, n->key)) {
      auto [newLeft, ptr] = insert_(n->left, k);
      n->left = newLeft;
      return {balance(n), ptr};
    }
    if (cmp_(n->key, k)) {
      auto [newRight, ptr] = insert_(n->right, k);
      n->right = newRight;
      return {balance(n), ptr};
    }
    // ya existe
    return {n, &n->val};
  }

  template <typename V>
  std::pair<Node *, Value *> insertVal_(Node *n, const Key &k, V &&v,
                                        bool &inserted) {
    if (!n) {
      ++size_;
      inserted = true;
      Node *nd = new Node(k, std::forward<V>(v));
      return {nd, &nd->val};
    }
    if (cmp_(k, n->key)) {
      auto [newLeft, ptr] =
          insertVal_(n->left, k, std::forward<V>(v), inserted);
      n->left = newLeft;
      return {balance(n), ptr};
    }
    if (cmp_(n->key, k)) {
      auto [newRight, ptr] =
          insertVal_(n->right, k, std::forward<V>(v), inserted);
      n->right = newRight;
      return {balance(n), ptr};
    }
    // ya existe → sobrescribir
    n->val = std::forward<V>(v);
    return {n, &n->val};
  }

  // ── Búsqueda ──────────────────────────────────────────────────────────────
  Node *find_(Node *n, const Key &k) const {
    while (n) {
      if (cmp_(k, n->key))
        n = n->left;
      else if (cmp_(n->key, k))
        n = n->right;
      else
        return n;
    }
    return nullptr;
  }

  // ── Nodo mínimo (para erase) ──────────────────────────────────────────────
  static Node *minNode(Node *n) {
    while (n->left)
      n = n->left;
    return n;
  }

  // ── Borrado ───────────────────────────────────────────────────────────────
  Node *erase_(Node *n, const Key &k, bool &erased) {
    if (!n)
      return nullptr;
    if (cmp_(k, n->key)) {
      n->left = erase_(n->left, k, erased);
    } else if (cmp_(n->key, k)) {
      n->right = erase_(n->right, k, erased);
    } else {
      erased = true;
      --size_;
      if (!n->left || !n->right) {
        Node *child = n->left ? n->left : n->right;
        delete n;
        return child;
      }
      Node *succ = minNode(n->right);
      n->key = std::move(succ->key);
      n->val = std::move(succ->val);
      bool dummy = false;
      n->right = erase_(n->right, n->key, dummy);
      ++size_; // compensar el --size_ del sucesor
    }
    return balance(n);
  }

  // ── Destrucción ───────────────────────────────────────────────────────────
  static void destroy(Node *n) {
    if (!n)
      return;
    destroy(n->left);
    destroy(n->right);
    delete n;
  }

  // ── Copia profunda ────────────────────────────────────────────────────────
  static Node *clone(const Node *n) {
    if (!n)
      return nullptr;
    Node *c = new Node(n->key, n->val);
    c->height = n->height;
    c->left = clone(n->left);
    c->right = clone(n->right);
    return c;
  }

public:
  // ── Iterador in-order ─────────────────────────────────────────────────────
  // Implementación con stack implícito (Morris sería O(1) espacio, pero más
  // complejo; stack es suficiente para colecciones de tamaño típico).
  struct Iterator {
    // Recorrido in-order con stack explícito
    struct Frame {
      Node *n;
      bool visited;
    };
    // Usamos un pequeño vector como pila
    std::vector<Node *> stk;
    Node *cur;

    explicit Iterator(Node *root) : cur(nullptr) {
      pushLeft(root);
      advance();
    }
    Iterator() : cur(nullptr) {}

    void pushLeft(Node *n) {
      while (n) {
        stk.push_back(n);
        n = n->left;
      }
    }
    void advance() {
      if (stk.empty()) {
        cur = nullptr;
        return;
      }
      cur = stk.back();
      stk.pop_back();
      pushLeft(cur->right);
    }

    std::pair<const Key &, Value &> operator*() const {
      return {cur->key, cur->val};
    }
    // Permite acceso estilo it->first / it->second mediante proxy
    struct Proxy {
      const Key &first;
      Value &second;
    };
    Proxy operator->() const { return {cur->key, cur->val}; }

    Iterator &operator++() {
      advance();
      return *this;
    }
    bool operator==(const Iterator &o) const { return cur == o.cur; }
    bool operator!=(const Iterator &o) const { return cur != o.cur; }
  };

  struct ConstIterator {
    std::vector<const Node *> stk;
    const Node *cur;

    explicit ConstIterator(const Node *root) : cur(nullptr) {
      pushLeft(root);
      advance();
    }
    ConstIterator() : cur(nullptr) {}

    void pushLeft(const Node *n) {
      while (n) {
        stk.push_back(n);
        n = n->left;
      }
    }
    void advance() {
      if (stk.empty()) {
        cur = nullptr;
        return;
      }
      cur = stk.back();
      stk.pop_back();
      pushLeft(cur->right);
    }

    struct Proxy {
      const Key &first;
      const Value &second;
    };
    Proxy operator->() const { return {cur->key, cur->val}; }
    std::pair<const Key &, const Value &> operator*() const {
      return {cur->key, cur->val};
    }

    ConstIterator &operator++() {
      advance();
      return *this;
    }
    bool operator==(const ConstIterator &o) const { return cur == o.cur; }
    bool operator!=(const ConstIterator &o) const { return cur != o.cur; }
  };

  Iterator begin() { return Iterator(root_); }
  Iterator end() { return Iterator(); }
  ConstIterator begin() const { return ConstIterator(root_); }
  ConstIterator end() const { return ConstIterator(); }

  // ── Big Five ──────────────────────────────────────────────────────────────
  AVLMap() = default;

  AVLMap(const AVLMap &o) : size_(o.size_), cmp_(o.cmp_) {
    root_ = clone(o.root_);
  }
  AVLMap(AVLMap &&o) noexcept
      : root_(o.root_), size_(o.size_), cmp_(std::move(o.cmp_)) {
    o.root_ = nullptr;
    o.size_ = 0;
  }
  ~AVLMap() { destroy(root_); }

  AVLMap &operator=(const AVLMap &o) {
    if (this != &o) {
      destroy(root_);
      root_ = clone(o.root_);
      size_ = o.size_;
      cmp_ = o.cmp_;
    }
    return *this;
  }
  AVLMap &operator=(AVLMap &&o) noexcept {
    if (this != &o) {
      destroy(root_);
      root_ = o.root_;
      size_ = o.size_;
      cmp_ = std::move(o.cmp_);
      o.root_ = nullptr;
      o.size_ = 0;
    }
    return *this;
  }

  // ── API pública ───────────────────────────────────────────────────────────

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  void clear() {
    destroy(root_);
    root_ = nullptr;
    size_ = 0;
  }

  // operator[] — crea entrada si no existe (igual que std::map)
  Value &operator[](const Key &k) {
    auto [newRoot, ptr] = insert_(root_, k);
    root_ = newRoot;
    return *ptr;
  }

  // insert con valor (copia o move) — sobrescribe si ya existe
  template <typename V> void insert(const Key &k, V &&v) {
    bool inserted = false;
    auto [newRoot, ptr] = insertVal_(root_, k, std::forward<V>(v), inserted);
    root_ = newRoot;
    (void)ptr;
  }

  // find — devuelve Iterator al nodo o end()
  Iterator find(const Key &k) {
    Node *n = find_(root_, k);
    if (!n)
      return end();
    // Construir iterador apuntando a n
    Iterator it;
    it.cur = n;
    // El stack no se necesita si solo se usa para comparar / desreferenciar
    // una vez; lo dejamos vacío (++it desde aquí no sería correcto, pero
    // find() típicamente solo se compara con end() y se desreferencia).
    // Para soporte completo de ++ tras find(), llenamos el stack in-order
    // desde la raíz hasta n (camino derecho).
    // — reconstruir stack desde root hasta n —
    Node *cur = root_;
    while (cur && cur != n) {
      if (cmp_(k, cur->key)) {
        it.stk.push_back(cur); // visitar cur después del subárbol izq
        // Pero n está en el subárbol izq, así que no empujamos cur todavía
        // Necesitamos la lógica in-order real:
        // Rehacemos: vaciamos y usamos pushLeft desde root hasta n
        it.stk.clear();
        break;
      } else {
        cur = cur->right;
      }
    }
    // Llenado correcto del stack para soportar ++ después de find:
    it.stk.clear();
    it.cur = nullptr;
    // Simulamos el recorrido in-order hasta llegar a n
    Node *p = root_;
    while (p) {
      if (cmp_(k, p->key)) {
        it.stk.push_back(p);
        p = p->left;
      } else if (cmp_(p->key, k)) {
        p = p->right;
      } else {
        // p == n
        it.cur = p;
        it.pushLeft(p->right); // siguientes en in-order
        break;
      }
    }
    return it;
  }

  ConstIterator find(const Key &k) const {
    const Node *n = find_(root_, k);
    if (!n)
      return end();
    ConstIterator it;
    const Node *p = root_;
    while (p) {
      if (cmp_(k, p->key)) {
        it.stk.push_back(p);
        p = p->left;
      } else if (cmp_(p->key, k)) {
        p = p->right;
      } else {
        it.cur = p;
        it.pushLeft(p->right);
        break;
      }
    }
    return it;
  }

  // count — 0 ó 1 (como std::map)
  size_t count(const Key &k) const { return find_(root_, k) ? 1 : 0; }

  // at — lanza si no existe
  Value &at(const Key &k) {
    Node *n = find_(root_, k);
    if (!n)
      throw std::out_of_range("AVLMap::at");
    return n->val;
  }
  const Value &at(const Key &k) const {
    const Node *n = find_(root_, k);
    if (!n)
      throw std::out_of_range("AVLMap::at");
    return n->val;
  }

  // erase por clave — devuelve número de elementos borrados (0 ó 1)
  size_t erase(const Key &k) {
    bool erased = false;
    root_ = erase_(root_, k, erased);
    return erased ? 1 : 0;
  }

  // erase por iterador — borra el elemento al que apunta it
  Iterator erase(Iterator it) {
    if (!it.cur)
      return end();
    Key k = it.cur->key;
    ++it; // avanzar antes de borrar
    erase(k);
    return it;
  }

  // reserve — no-op (AVL no usa buckets, se deja para compatibilidad de firma)
  void reserve(size_t) {}
};

class Fecha {
  friend bool operator==(const Fecha &a, const Fecha &b);
  friend bool operator<(const Fecha &a, const Fecha &b);
  friend bool operator>(const Fecha &a, const Fecha &b);
  friend bool operator<=(const Fecha &a, const Fecha &b);
  friend bool operator>=(const Fecha &a, const Fecha &b);
  friend bool operator!=(const Fecha &a, const Fecha &b);
  friend std::ostream &operator<<(std::ostream &s, const Fecha &f);

public:
  Fecha();
  Fecha(int d, int m, int y);
  Fecha(Fecha &&) = default;
  Fecha(const Fecha &) = default;
  Fecha &operator=(Fecha &&) = default;
  Fecha &operator=(const Fecha &) = default;
  ~Fecha();

  int getDay() const { return day; }
  int getMonth() const { return month; }
  int getYear() const { return year; }

  void setDay(int d);
  void setMonth(int m);
  void setYear(int y);

private:
  int day;
  int month;
  int year;
};

class InfTermDoc {
  friend ostream &operator<<(ostream &s, const InfTermDoc &p);

public:
  InfTermDoc(const InfTermDoc &) = default;
  InfTermDoc(InfTermDoc &&) = default; // ── OPTIMIZACIÓN: move ctor
  InfTermDoc();
  ~InfTermDoc() = default;
  InfTermDoc &operator=(const InfTermDoc &) = default;
  InfTermDoc &
  operator=(InfTermDoc &&) = default; // ── OPTIMIZACIÓN: move assign

  int getFt() const { return ft; }
  const vector<int> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  // ── OPTIMIZACIÓN: move overload para evitar copia del vector ─────────────
  void setPosTerm(const vector<int> &p) { posTerm = p; }
  void setPosTerm(vector<int> &&p) { posTerm = std::move(p); }

  void incFt() { ft++; }
  void incPosTerm(int p) { posTerm.push_back(p); }

private:
  int ft;
  vector<int> posTerm;
};

class InformacionTermino {
  friend std::ostream &operator<<(std::ostream &s, const InformacionTermino &p);

public:
  // ── Big Five (todos por defecto: AVLMap ya los implementa) ────────────────
  InformacionTermino() = default;
  ~InformacionTermino() = default;
  InformacionTermino(const InformacionTermino &) = default;
  InformacionTermino(InformacionTermino &&) = default;
  InformacionTermino &operator=(const InformacionTermino &) = default;
  InformacionTermino &operator=(InformacionTermino &&) = default;

  // ── Getters ───────────────────────────────────────────────────────────────
  int getFtc() const { return ftc; }

  // Devuelve copia del mapa (compatibilidad con código que lo usaba así)
  AVLMap<uint16_t, InfTermDoc> getLdocs() const { return l_docs; }

  // Referencia constante al mapa (lectura eficiente)
  const AVLMap<uint16_t, InfTermDoc> &getL_docs() const { return l_docs; }

  // Referencia mutable (para modificar entradas existentes)
  AVLMap<uint16_t, InfTermDoc> &getL_docs_mut() { return l_docs; }

  // ── Setters ───────────────────────────────────────────────────────────────
  inline void setFtc(int f) { ftc = f; }
  inline void setL_docs(const AVLMap<uint16_t, InfTermDoc> &l) { l_docs = l; }

  // ── Mutadores ─────────────────────────────────────────────────────────────
  inline void incFtc() { ++ftc; }

  // Insertar / sobrescribir con move (ruta rápida en IndexarFichero)
  void addL_docs(uint16_t a, InfTermDoc &&l) { l_docs.insert(a, std::move(l)); }
  // Insertar / sobrescribir con copia
  void addL_docs(int a, const InfTermDoc &l) {
    l_docs.insert(static_cast<uint16_t>(a), l);
  }

  // ── Búsqueda directa (evita exponer el mapa cuando solo se necesita find) ──
  // Devuelve puntero al valor o nullptr si no existe.
  InfTermDoc *findDoc(uint16_t id) {
    auto it = l_docs.find(id);
    if (it == l_docs.end())
      return nullptr;
    return &(*it).second;
  }
  const InfTermDoc *findDoc(uint16_t id) const {
    auto it = l_docs.find(id);
    if (it == l_docs.end())
      return nullptr;
    return &(*it).second;
  }

private:
  int ftc = 0;
  AVLMap<uint16_t, InfTermDoc> l_docs;
};

class InfDoc {
  friend ostream &operator<<(ostream &s, const InfDoc &p);

public:
  InfDoc(const InfDoc &) = default;
  InfDoc(InfDoc &&) = default; // ── OPTIMIZACIÓN
  InfDoc(const string &);
  InfDoc();
  ~InfDoc() = default;
  InfDoc &operator=(const InfDoc &) = default;
  InfDoc &operator=(InfDoc &&) = default; // ── OPTIMIZACIÓN

  int getidDoc() const;

  int getNumPal() const { return numPal; }
  int getNumPalSinParada() const { return numPalSinParada; }
  int getNumPalDiferentes() const { return numPalDiferentes; }
  int getTamBytes() const { return tamBytes; }
  const Fecha &getFechaModificacion() const { return fechaModificacion; }

  void setIdDoc(int id) { idDoc = id; }
  void setNumPal(int n) { numPal = n; }
  void setNumPalSinParada(int n) { numPalSinParada = n; }
  void setNumPalDiferentes(int n) { numPalDiferentes = n; }

  void incNumPal() { numPal++; }
  void incNumPalSinParada() { numPalSinParada++; }
  void incNumPalDiferentes() { numPalDiferentes++; }

  void setTamBytes(int t) { tamBytes = t; }
  void setFechaModificacion(const Fecha &f) { fechaModificacion = f; }

private:
  uint16_t idDoc;
  int numPal;
  int numPalSinParada;
  int numPalDiferentes;
  int tamBytes;
  Fecha fechaModificacion;
};

class InfColeccionDocs {
  friend ostream &operator<<(ostream &s, const InfColeccionDocs &p);

public:
  InfColeccionDocs(const InfColeccionDocs &) = default;
  InfColeccionDocs(InfColeccionDocs &&) = default;
  InfColeccionDocs();
  ~InfColeccionDocs() = default;
  InfColeccionDocs &operator=(const InfColeccionDocs &) = default;
  InfColeccionDocs &operator=(InfColeccionDocs &&) = default;

  int getNumDocs() const { return numDocs; }
  int getNumTotalPal() const { return numTotalPal; }
  int getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  int getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }
  int getTamBytes() const { return tamBytes; }

  void setNumDocs(int n) { numDocs = n; }
  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }
  void setTamBytes(int t) { tamBytes = t; }

private:
  uint16_t numDocs;
  int numTotalPal;
  int numTotalPalSinParada;
  int numTotalPalDiferentes;
  int tamBytes;
};

class InformacionTerminoPregunta {
  friend ostream &operator<<(ostream &s, const InformacionTerminoPregunta &p);

public:
  InformacionTerminoPregunta(const InformacionTerminoPregunta &) = default;
  InformacionTerminoPregunta(InformacionTerminoPregunta &&) = default;
  InformacionTerminoPregunta();
  ~InformacionTerminoPregunta() = default;
  InformacionTerminoPregunta &
  operator=(const InformacionTerminoPregunta &) = default;
  InformacionTerminoPregunta &
  operator=(InformacionTerminoPregunta &&) = default;

  int getFt() const { return ft; }
  const vector<uint16_t> &getPosTerm() const { return posTerm; }

  void setFt(int f) { ft = f; }
  void incFt() { ft++; }
  void setPosTerm(const vector<uint16_t> &p) { posTerm = p; }
  void addPosTerm(int pos) { posTerm.push_back(pos); }

private:
  int ft;
  vector<uint16_t> posTerm;
};

class InformacionPregunta {
  friend ostream &operator<<(ostream &s, const InformacionPregunta &p);

public:
  InformacionPregunta(const InformacionPregunta &) = default;
  InformacionPregunta(InformacionPregunta &&) = default;
  InformacionPregunta();
  ~InformacionPregunta() = default;
  InformacionPregunta &operator=(const InformacionPregunta &) = default;
  InformacionPregunta &operator=(InformacionPregunta &&) = default;

  uint16_t getNumTotalPal() const { return numTotalPal; }
  uint16_t getNumTotalPalSinParada() const { return numTotalPalSinParada; }
  uint16_t getNumTotalPalDiferentes() const { return numTotalPalDiferentes; }

  void setNumTotalPal(int n) { numTotalPal = n; }
  void setNumTotalPalSinParada(int n) { numTotalPalSinParada = n; }
  void setNumTotalPalDiferentes(int n) { numTotalPalDiferentes = n; }

private:
  uint16_t numTotalPal;
  uint16_t numTotalPalSinParada;
  uint16_t numTotalPalDiferentes;
};

#endif
