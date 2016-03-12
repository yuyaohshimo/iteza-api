require 'rest-client'
require 'json'

require File.expand_path(File.dirname(__FILE__) + '/../modules/send_mail')

class DepositJob
  include SendMail

  API_URL = "http://www.fiftyriver.net:3000/api/check/checkin"

  def receive(message, params)
    account_no = message.body.decoded.match(/[0-9]{10}/)[0]
    receiver = message.from_addrs[0]

    begin
      response = RestClient.post API_URL,
        { account_no: account_no, receiver: receiver }.to_json,
        content_type: :json,
        accept: :json

      puts JSON.parse(response)

      send_deposit_confirm(receiver)
    rescue => e
      puts e.message
    end
  end
end