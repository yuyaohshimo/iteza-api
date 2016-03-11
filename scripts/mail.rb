#!/bin/env ruby
# -*- coding: utf-8 -*-
# メール送受信スクリプト 突貫工事


require 'net/imap'
require 'kconv'
require 'mail'

imap_usessl = true

imap_host = 'imap4.muumuu-mail.com'
imap_port = 993

# imapのユーザ名とパスワード
imap_user = 'get@checky.me'
imap_passwd = 'XXXX'

search_criterias = ['UNSEEN']

imap = Net::IMAP.new(imap_host, imap_port, imap_usessl)
imap.login(imap_user, imap_passwd)
imap.examine('INBOX')

nyukinmsg = <<MSGEND
入金依頼を受け付けました。入金先の口座番号を教えて下さい。
---
送信例）ufj しぶや 123457
MSGEND

# smtp設定
Mail.defaults do
  delivery_method :smtp, { address:   'smtp.muumuu-mail.com',
                           port:      465,
                           domain:    'checky.me',
                           user_name: 'get@checky.me',
                           password:  'XXXX',
                           authentication: 'plain',
                           ssl: true
                         }

end

# 未読メールを検索
imap.search(search_criterias).each do |msg_id|
    msg = imap.fetch(msg_id, "RFC822").first
    m = Mail.new(msg.attr["RFC822"])
    body = m.body.decoded.toutf8.strip

    # debug
    puts m.subject, m.from, body

    if body =~ /入金/ then
      Mail.deliver do
        from    'get@checky.me'
        to      m.from
        subject 'Mail from Checky'
        body    nyukinmsg
      end
    end

    imap.store(msg_id, '+FLAGS', :Seen)
end

imap.logout
