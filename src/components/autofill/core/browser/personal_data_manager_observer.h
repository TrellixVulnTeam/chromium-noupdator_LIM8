// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_H_

namespace autofill {

// An interface the PersonalDataManager uses to notify its clients (observers)
// when it has finished loading personal data from the web database.  Register
// observers via PersonalDataManager::AddObserver.
class PersonalDataManagerObserver {
 public:
  // Notifies the observer that the PersonalDataManager changed in some way.
  virtual void OnPersonalDataChanged() {}

  // Called when there is insufficient data to fill a form. Used for testing.
  virtual void OnInsufficientFormData() {}

  // Notifies the observer that the PersonalDataManager has no more tasks to
  // handle.
  virtual void OnPersonalDataFinishedProfileTasks() {}

  // Notifies the observer whenever at least one (can be multiple) credit card
  // is suceesfully saved.
  // TODO(crbug.com/964127): Need to add a bool to separate server card save and
  // local card save to decide whether to show the Autofill sign-in after local
  // save promo bubble.
  virtual void OnCreditCardSaved() {}

 protected:
  virtual ~PersonalDataManagerObserver() {}
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_PERSONAL_DATA_MANAGER_OBSERVER_H_
