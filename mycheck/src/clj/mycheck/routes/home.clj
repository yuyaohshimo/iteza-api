(ns mycheck.routes.home
  (:require [mycheck.layout :as layout]
            [compojure.core :refer [defroutes GET]]
            [ring.util.http-response :as response]
            [clojure.java.io :as io]))

(def authurl
  [:safe
    (str "https://demo-ap08-prod.apigee.net/oauth/authorize?"
      "redirect_uri=" "http://www.fiftyriver.net:3000/auth"
      "&"
      "client_id=" "4kSp7wOSg8Vfg6KIQHZ6wPQi5j4XkaCX"
      "&"
      "response_type=token")])

(defn home-page []
  (layout/render
    "home.html" {:authurl authurl}))

(defn auth-page []
  (layout/render "auth.html"))

(defn about-page []
  (layout/render "about.html"))

(defn docs-page []
  (layout/render "docs.html"
           {:docs (-> "docs/docs.md" io/resource slurp)}))

(defroutes home-routes
  (GET "/" [] (home-page))
  (GET "/auth" [] (auth-page))
  (GET "/about" [] (about-page))
  (GET "/docs" [] (docs-page)))
