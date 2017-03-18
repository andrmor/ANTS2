#ifndef ALRFTYPE_H
#define ALRFTYPE_H

#include <memory>
#include <vector>

#include "idclasses.h"
#include "apoint.h"

class QJsonObject;
class QWidget;

class AStateInterface;

namespace LRF {

class ALrf;
class ALrfSettingsWidget;
class ALrfInternalsWidget;

///
/// \brief Provides methods to manipulate ALrf derived class(es).
///
class ALrfType {
  ALrfTypeID id_;
  std::string name_;
protected:

  ///
  /// \brief Method to save.
  /// \details Is called by ALrfTypeManager to enforce proper ["type"] in json.
  /// \param lrf
  /// \return
  ///
  virtual QJsonObject lrfToJson(const ALrf *lrf) const = 0;

  ///
  /// \brief Method to load
  /// \details Is called by ALrfTypeManager to enforce proper ALrfType from
  ///  ["type"] in json.
  /// \param json
  /// \return
  ///
  virtual ALrf *lrfFromJson(const QJsonObject &json) const = 0;

public:
  ALrfType(const std::string &name) : name_(name) {}
  ALrfTypeID id() const { return id_; }
  virtual ~ALrfType() {}
  const std::string &name() const { return name_; }
  virtual std::string nameUi() const { return name_; }

  virtual bool isCudaCapable() const { return false; }
  virtual void getCudaParameters(std::shared_ptr<const ALrf> /*lrf*/) { }

#ifdef GUI
  ///
  /// \brief Method to configure settings for the fit layer instruction.
  /// \details The returned object should emit elementChanged() signals whenever
  /// one of it's child objects is changed.
  /// \return A QWidget* which can be dynamic_cast<AStateInterface*>'ed to allow
  ///  to save/load the returned widget's state. The state object is also used
  ///  in ALrfType::lrfFromData().
  ///
  virtual QWidget *newSettingsWidget(QWidget *parent = 0) const = 0;

  ///
  /// \brief Shows the internal values of an Lrf.
  /// \details If two Lrfs have the same internal values' state, they should
  ///  produce the same results in all circumstances. The returned object should
  ///  emit elementChanged() signals whenever one of it's child objects is changed.
  /// \return A QWidget* which can be dynamic_cast<AStateInterface*>'ed to allow
  ///  to save/load the returned widget's state. The state object is used in
  ///  ARepository::lrfToJson() and ARepository::lrfFromJson().
  ///
  virtual QWidget *newInternalsWidget(QWidget *parent = 0) const = 0;
#endif

  ///
  /// \brief Method to make
  /// \param settings
  /// \param event_pos
  /// \param event_signal
  /// \return
  ///
  virtual ALrf *lrfFromData(const QJsonObject &settings, bool fit_error,
                            const std::vector<APoint> &event_pos,
                            const std::vector<double> &event_signal) const = 0;

  ///
  /// \brief Copies lrf to the caller's thread context, such that the return
  ///  value is equal (defined by operator==) to lrf.
  /// \details The caller's thread may assume it has the only copy of the
  ///  returned object, and thus may use it considering only a single-thread
  ///  model. Thread-safe types don't need to implement this.
  /// \param lrf The lrf to be copied
  /// \return A copy of lrf (lrf == returned_value is true) in the current
  ///  thread context.
  ///
  virtual std::shared_ptr<const ALrf> copyToCurrentThread(std::shared_ptr<const ALrf> lrf) const
  { return lrf; }

  friend class ALrfTypeManager;
};

}

#endif // ALRFTYPE_H
