require File.expand_path(File.dirname(__FILE__) + '/../modules/send_mail')

class DepositConfirmJob
  include SendMail

  def receive(message, params)
    receiver = message.from_addrs[0]
    send_deposit_success(receiver)
  end
end