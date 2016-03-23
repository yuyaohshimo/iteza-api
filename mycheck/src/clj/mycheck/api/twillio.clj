(ns mycheck.api.twillio
    (:require [twilio.core :as tw]
              [config.core :refer [env]]))

;; Twillio send sms
(defn send-sms [message to]
  (tw/with-auth (:twillio-sid env) (:twillio-token env)
    (tw/send-sms {:From "+16463927087"
                  :To to
                  :Body message})))
