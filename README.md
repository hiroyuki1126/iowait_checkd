# iowait_checkd

## ツールのインストール方法

* `root`ユーザで以下を実行する。

```
# git clone https://github.com/hiroyuki1126/iowait_checkd
# cd iowait_checkd
# make install clean
```

## ツールの実行方法

* 本ツールは、デーモンプロセスとして実行される。
* `root`ユーザで以下を実行する。

```
# cd /root/bin
# ./iowait_checkd.sh <サブコマンド>
```

| サブコマンド | 説明 |
| ----- | ----- |
| `start` | ツールを実行開始する。 |
| `stop` | ツールを実行停止する。 |
| `restart` | ツールを再起動する。 |
| `reload`| 実行中のツールに対して、パラメータファイルを再読み込みさせる。 |
| `status` | ツールの実行状態を確認する。 |

## パラメータファイル

* パラメータは、`<パラメータ名> = <値>` の形式で設定する（イコールの前後にスペースが必要）。
* 行頭に`#`を付けることでコメントアウトできる。
* パラメータを記載しない場合は、デフォルト値が適用される。
* パラメータは、ツールの起動時だけでなく、起動中に動的変更（`reload`）することもできる。

| パラメータ | デフォルト値 | 説明 |
| ----- | ----- | ----- |
| `iowait_check_intvl` | `5` | iowaitの監視間隔を秒単位で指定する。 |
| `iowait_thresh` | `30` | iowaitの監視閾値をパーセント単位で指定する。 |
| `iotop_output_file` | `/tmp/iowait_checkd_iotop.log` | ログファイルのパスを指定する。 |
| `iotop_output_file_bkp` | `/tmp/iowait_checkd_iotop.log.1` | ローテーションされたログファイルのパスを指定する。 |
| `iotop_output_file_size_max` | `100000000` | 出力されるログファイルの最大サイズをByte単位で指定する。 |

## 補足

* ツール実行中に何らかのエラーが発生した場合は、`/var/log/messages`にエラーメッセージが出力される。
* `iowait_checkd.sh reload`コマンドを実行すると、変更前後のパラメータ値が`/var/log/messages`に出力される。
